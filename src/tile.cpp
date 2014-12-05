#include "tile.hpp"
#include "vector_tile.pb.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>

namespace avecado {

tile::tile(unsigned int z_, unsigned int x_, unsigned int y_)
  : z(z_), x(x_), y(y_), m_mapnik_tile(new mapnik::vector::tile) {
}

tile::~tile() {
}

std::string tile::get_data() const {
  std::ostringstream buffer;
  buffer << *this;
  return buffer.str();
}

void tile::from_string(const std::string &str) {
  tile t(z, x, y);
  std::istringstream buffer(str);
  buffer >> t;
  m_mapnik_tile.swap(t.m_mapnik_tile);
}

mapnik::vector::tile const &tile::mapnik_tile() const {
  return *m_mapnik_tile;
}

mapnik::vector::tile &tile::mapnik_tile() {
  return *m_mapnik_tile;
}

std::istream &operator>>(std::istream &in, tile &t) {
  google::protobuf::io::IstreamInputStream stream(&in);
  bool read_ok = t.mapnik_tile().ParseFromZeroCopyStream(&stream);

  if (!read_ok) {
    throw std::runtime_error("Unable to read tile from input stream.");
  }

  return in;
}

std::ostream &operator<<(std::ostream &out, const tile &t) {
  google::protobuf::io::OstreamOutputStream stream(&out);
  bool write_ok = t.mapnik_tile().SerializeToZeroCopyStream(&stream);

  if (!write_ok) {
    throw std::runtime_error("Unable to write tile to output stream.");
  }

  return out;
}

} // namespace avecado
