#pragma once

#include <QDate>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoRectangle>
#include <QMediaPlayer>
#include <QUrl>
#include <mgps/drive.hh>

#include "QmlObject.hh"

namespace mgps::declarative {
	class QmlDrive : public QmlObject {
		Q_OBJECT
		Q_PROPERTY(
		    QGeoRectangle visibleRegion READ visibleRegion NOTIFY driveChanged)
		Q_PROPERTY(long long playback READ playback WRITE setPlayback NOTIFY
		               playbackChanged)
		Q_PROPERTY(QString playbackString READ playbackString NOTIFY
		               playbackStringChanged)
		Q_PROPERTY(long long duration READ duration NOTIFY driveChanged)
		Q_PROPERTY(
		    QString durationString READ durationString NOTIFY driveChanged)
		Q_PROPERTY(QDateTime timeline READ timeline NOTIFY timelineChanged)
		Q_PROPERTY(QString timelineString READ timelineString NOTIFY
		               timelineStringChanged)
		Q_PROPERTY(QGeoCoordinate carPosition READ carPosition NOTIFY
		               carPositionChanged)
		Q_PROPERTY(QVariantList lines READ lines NOTIFY driveChanged)
	public:
		QmlDrive();

		void setDrive(mgps::drive const* drive);

		Q_INVOKABLE void playerAvailable(QObject*);

		void setPlayback(unsigned long long milliseconds, bool forced = false);

		QGeoRectangle visibleRegion() const noexcept;
		long long playback() const noexcept {
			return playback_.time_since_epoch().count();
		}
		long long duration() const noexcept { return duration_.count(); }
		QDateTime timeline() const noexcept;
		QGeoCoordinate const& carPosition() const noexcept {
			return car_position_;
		}
		QVariantList const& lines() const noexcept { return lines_; }

		QString playbackString() const;
		QString durationString() const;
		QString timelineString() const;

	signals:
		void driveChanged();
		void playbackChanged();
		void playbackStringChanged();
		void timelineChanged();
		void timelineStringChanged();
		void carPositionChanged();

	private:
		using milliseconds = std::chrono::milliseconds;
		void populatePlaylist();
		void populateLines();
		size_t currentIndex();
		travel_ms playbackToTimeline(playback_ms) const;
		local_milliseconds timelineToLocal(travel_ms) const;
		QGeoCoordinate timelineToPosition(travel_ms) const;

		playback_ms getOffset(size_t currentFile) {
			if (currentFile >= offsets_.size()) {
				if (offsets_.empty()) return {};
				return offsets_.back();
			}
			return offsets_[currentFile];
		}

		mgps::drive const* drive_{nullptr};
		ch::milliseconds duration_{};
		playback_ms playback_{};
		travel_ms timeline_{};
		QGeoCoordinate car_position_{};
		QVariantList lines_{};
		std::vector<playback_ms> offsets_{};
		QMediaPlayer* player_{nullptr};
	};
}  // namespace mgps::declarative

Q_DECLARE_METATYPE(mgps::declarative::QmlDrive);
