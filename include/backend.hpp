#ifndef AVECADO_BACKEND_HPP
#define AVECADO_BACKEND_HPP

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/vertex.hpp>
#include <mapnik/map.hpp>
#include <mapnik/path.hpp>

// vector tile
#include "vector_tile_backend_pbf.hpp"

// boost
#include <boost/optional.hpp>

namespace avecado {

class post_processor;

namespace detail {
struct fake_feature {
  inline void set_type(::vector_tile::Tile_GeomType value) { m_type = value; }
  fake_feature *operator->() { return this; }
  ::vector_tile::Tile_GeomType m_type;
};
}

class backend {
public:
  backend(vector_tile::Tile & tile,
          unsigned path_multiplier,
          mapnik::Map const& map,
          boost::optional<const post_processor &> pp);

  inline unsigned get_path_multiplier() { return m_pbf.get_path_multiplier(); }

  void start_tile_layer(std::string const& name);

  void stop_tile_layer();

  void start_tile_feature(mapnik::feature_impl const& feature);

  void stop_tile_feature();

  void add_tile_feature_raster(std::string const& image_buffer);

  template <typename T>
  inline unsigned add_path(const T &path) {
    m_current_geometry_collection.push_back(path);
    return 1;
  }

  // yuck - but needed for compatibility?
  detail::fake_feature current_feature_;

private:
  mapnik::vector_tile_impl::backend_pbf m_pbf;
  mapnik::Map const& m_map;
  unsigned int m_tolerance;
  boost::optional<const post_processor &> m_post_processor;
  std::string m_current_layer_name;
  std::vector<mapnik::feature_ptr> m_current_layer_features;
  mapnik::geometry::geometry_collection<std::int64_t> m_current_geometry_collection;
  mapnik::feature_ptr m_current_feature;
  boost::optional<std::string> m_current_image_buffer;
};

} // namespace avecado

#endif // AVECADO_BACKEND_HPP
