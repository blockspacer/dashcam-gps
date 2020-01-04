#include "qml_trip.hh"

#include <date/chrono_io.h>
#include <QDebug>
#include <QLocale>

using namespace std::chrono;
using namespace date;
using namespace mgps;

QmlTrip::QmlTrip() {}

QGeoRectangle QmlTrip::visibleRegion() const noexcept {
	if (!trip_) return QGeoRectangle{};
	static constexpr auto MARGIN = 0.005;
	auto const bounds = trip_->plot.boundary_box();
	auto const center = bounds.center();
	auto const width = bounds.width().as_float() + 2 * MARGIN;
	auto const height = bounds.height().as_float() + 2 * MARGIN;
	return {QGeoCoordinate{center.lat.as_float(), center.lon.as_float()}, width,
	        height};
}

QDateTime QmlTrip::timeline() const noexcept {
	auto start = [](auto* jump) {
		return jump ? jump->start : local_milliseconds{};
	};
	auto const secs = floor<seconds>(start(trip_) + timeline_);
	auto const date = floor<days>(secs);
	auto const time = secs - date;

	auto const ymd = year_month_day{local_days{date}};
	auto const hms = hh_mm_ss{time};
	return {QDate{int(ymd.year()), int(unsigned(ymd.month())),
	              int(unsigned(ymd.day()))},
			QTime{int(hms.hours().count()), int(hms.minutes().count()),
	              int(hms.seconds().count())}};
}

QString QmlTrip::playbackString() const {
	std::ostringstream ostr;
	ostr << hh_mm_ss{floor<seconds>(playback_)};
	return QString::fromStdString(ostr.str());
}

QString QmlTrip::durationString() const {
	std::ostringstream ostr;
	ostr << hh_mm_ss{floor<seconds>(duration_)};
	return QString::fromStdString(ostr.str());
}

QString QmlTrip::timelineString() const {
	auto const start = [&] {
		if (!trip_) return local_milliseconds{};
		return trip_->start;
	}();
	std::ostringstream ostr;
	ostr << floor<seconds>(start + timeline_);
	return QString::fromStdString(ostr.str());
}

void QmlTrip::setTrip(mgps::library::trip const* trip) {
	trip_ = trip;

	jumps_.clear();
	duration_ = {};

	if (trip_) {
		milliseconds timeline{};

		jumps_.reserve(trip_->strides.size());
		for (auto const& slice : trip_->strides) {
			if (timeline != slice.offset) {
				jumps_.push_back({duration_, slice.offset - duration_});
				qDebug() << "Jump at" << jumps_.back().playback.count()
				         << "ms to" << jumps_.back().fixup.count() << "ms";
			}
			duration_ += slice.duration;
			timeline = slice.offset + slice.duration;
		}
	}

	force_updates_ = true;
	setPlayback(0);
	force_updates_ = false;

	emit tripChanged();
}

void QmlTrip::setPlayback(unsigned long long new_millis) {
	auto const millis = milliseconds{new_millis};
	if (!force_updates_ && millis == playback_) return;

	auto const old_playback = floor<seconds>(playback_);
	playback_ = millis;

	milliseconds new_timeline{};
	QGeoCoordinate new_position{};

	new_timeline = millis;
	if (!jumps_.empty()) {
		auto it = std::upper_bound(begin(jumps_), end(jumps_), jump{millis, {}},
		                           [](auto const& lhs, auto const& rhs) {
			                           return lhs.playback < rhs.playback;
		                           });
		if (it != begin(jumps_)) {
			new_timeline += std::prev(it)->fixup;
		}
	}
	auto const emit_timeline = new_timeline != timeline_;
	auto const old_timeline = floor<seconds>(timeline_);
	timeline_ = new_timeline;

	if (trip_) {
		// TODO: support trips w/out GPS
		auto it_seg = std::upper_bound(begin(trip_->plot.segments),
		                               end(trip_->plot.segments),
		                               library::timeline_item{timeline_},
		                               [](auto const& lhs, auto const& rhs) {
			                               return lhs.offset < rhs.offset;
		                               });

		if (it_seg != begin(trip_->plot.segments)) std::advance(it_seg, -1);
		if (it_seg != end(trip_->plot.segments)) {
			auto const& seg = *it_seg;
			auto const time = [=, offset = seg.offset]() {
				library::track::gps_data out{};
				out.time = timeline_ - offset;
				return out;
			}();

			auto it = std::upper_bound(begin(seg.points), end(seg.points), time,
			                           [](auto const& lhs, auto const& rhs) {
				                           return lhs.time < rhs.time;
			                           });

			if (it != begin(seg.points)) std::advance(it, -1);
			if (it != end(seg.points)) {
				auto const& pt = *it;
				new_position = {pt.lat.as_float(), pt.lon.as_float()};
			}
		}
	}

	auto const emit_position = new_position != car_position_;
	car_position_ = new_position;

	emit playbackChanged();
	if (force_updates_ || floor<seconds>(playback_) != old_playback)
		emit playbackStringChanged();
	if (force_updates_ || emit_timeline) emit timelineChanged();
	if (force_updates_ || floor<seconds>(timeline_) != old_timeline)
		emit timelineStringChanged();
	if (force_updates_ || emit_position) emit positionChanged();
}
