#include <mgps/library.hh>
#include <mgps/trip.hh>

namespace mgps {
	void library::before_update() { footage_.clear(); }

	bool library::add_file(fs::path const& filename) {
		footage_.emplace_back();
		for (auto& plugin : plugins_) {
			if (!plugin->probe(filename)) continue;
			auto& back = footage_.back();
			if (!plugin->load(filename, &back)) {
				back = media_file{};
				continue;
			}
			return true;
		}
		footage_.pop_back();

		return false;
	}

	void library::add_directory(fs::path const& dirname) {
		std::error_code ec{};

		auto items = fs::directory_iterator{dirname, ec};
		if (ec) return;

		for (auto const& entry : items) {
			if (!entry.is_regular_file(ec) || ec) continue;
			add_file(entry.path());
		}
	}

	void library::after_update() {
		std::sort(begin(footage_), end(footage_),
		          [](auto const& lhs, auto const& rhs) {
			          return lhs.date_time < rhs.date_time;
		          });

		local_ms prev{};
		for (auto& clip : footage_) {
			if (prev > clip.date_time) clip.date_time = prev;
			prev = clip.date_time + clip.duration;
		}
	}

	namespace {
		inline bool is_compatible(clip type, page kind) {
			switch (type) {
				case clip::emergency:
					return kind != page::parking;
				case clip::parking:
					return kind != page::emergency;
				default:
					return kind == page::everything;
			}
		}

		inline void create_playlist(std::vector<size_t> const& indices,
		                            std::vector<media_file> const& footage,
		                            video::playlist& playlist) {
			playlist.media.clear();
			playlist.media.reserve(indices.size());

			ch::milliseconds playback{};

			for (auto index : indices) {
				auto& clip = footage[index];

				playlist.media.emplace_back();
				auto& ref = playlist.media.back();

				ref.offset = playback_ms{playback};
				ref.duration = clip.duration;
				ref.reference = index;

				playback += clip.duration;
			}

			playlist.duration = playback;
		}

		inline bool detached(track::gps_point const& previous,
		                     track::gps_point const& current) {
			return (current.offset - previous.offset) > ch::seconds{1};
		}

		inline void create_lines(video::playlist const& playlist,
		                         std::vector<media_file> const& footage,
		                         track::trace& trace) {
			{
				size_t reserve_for_lines{};
				track::gps_point const* previous = nullptr;

				for (auto const& file : playlist.media) {
					for (auto& point : footage[file.reference].points) {
						if (!previous || detached(*previous, point)) {
							++reserve_for_lines;
						}
						previous = &point;
					}
				}

				trace.lines.reserve(reserve_for_lines);
			}

			{
				size_t reserve_for_line{};
				track::gps_point const* previous = nullptr;

				for (auto const& file : playlist.media) {
					for (auto& point : footage[file.reference].points) {
						if (previous && detached(*previous, point)) {
							trace.lines.emplace_back();
							trace.lines.back().points.reserve(reserve_for_line);
							reserve_for_line = 0;
						}
						previous = &point;
						++reserve_for_line;
					}
				}

				if (reserve_for_line) {
					trace.lines.emplace_back();
					trace.lines.back().points.reserve(reserve_for_line);
				}
			}
			{
				size_t line_index{};
				track::gps_point const* previous = nullptr;

				for (auto const& file : playlist.media) {
					for (auto& input : footage[file.reference].points) {
						if (previous && detached(*previous, input)) {
							++line_index;
						}
						previous = &input;

						auto copy = input;
						copy.offset += file.offset.time_since_epoch();
						auto& line = trace.lines[line_index].points;
						if (line.empty() || line.back().offset != copy.offset)
							line.push_back(copy);
						else
							line.back() = copy;
					}
				}
			}

			for (auto& line : trace.lines) {
				using namespace std::literals;
				line.offset = playback_ms{line.points.front().offset};
				line.duration =
				    line.points.empty() ? 0ms : (line.points.size() - 1) * 1s;
				for (auto& point : line.points)
					point.offset -= line.offset.time_since_epoch();
			}

			if (trace.lines.empty()) {
				trace.offset = mgps::playback_ms{};
				trace.duration = ch::milliseconds{};
			} else {
				trace.offset = trace.lines.front().offset;
				for (auto& line : trace.lines)
					line.offset -= trace.offset.time_since_epoch();
				auto& last_line = trace.lines.back();
				trace.duration =
				    last_line.offset.time_since_epoch() + last_line.duration;
			}
		}
	}  // namespace

	std::vector<trip> library::build(page kind,
	                                 ch::milliseconds max_gap) const {
		std::vector<std::vector<size_t>> view{};

		size_t index = 0;
		local_ms prev{};
		for (auto& clip : footage_) {
			struct autoinc {
				size_t* ptr;
				~autoinc() { ++*ptr; }
			} raii{&index};

			if (!is_compatible(clip.type, kind)) continue;

			if (view.empty() || ((prev + max_gap) < clip.date_time))
				view.emplace_back();

			prev = clip.date_time + clip.duration;
			view.back().emplace_back(index);
		}

		std::vector<trip> collection;

		collection.reserve(view.size());
		for (auto& fragment : view) {
			collection.emplace_back();
			auto& trip = collection.back();
			trip.owner = this;

			trip.start = [](std::vector<size_t> const& indices,
			                std::vector<media_file> const& footage) {
				if (indices.empty()) return local_ms{};
				return footage[indices.front()].date_time;
			}(fragment, footage_);

			create_playlist(fragment, footage_, trip.playlist);
			create_lines(trip.playlist, footage_, trip.trace);
		}

		return collection;
	}

	media_file const* library::footage(video::media_clip const& ref) const
	    noexcept {
		if (ref.reference >= footage_.size()) return nullptr;
		return &footage_[ref.reference];
	}

}  // namespace mgps
