#include <boost/python.hpp>

#include "avecado.hpp"
#include "post_processor.hpp"
#include "util.hpp"
#include "../logging/logger.hpp"

namespace pt = boost::property_tree;
using namespace boost::python;

namespace {

str mk_tile(object py_map,
            unsigned int z,
            unsigned int x,
            unsigned int y,
            unsigned int path_multiplier,
            int buffer_size,
            double scale_factor,
            unsigned int offset_x,
            unsigned int offset_y,
            unsigned int tolerance,
            std::string image_format,
            std::string scaling_method_str,
            double scale_denominator,
            object post_processor) {

  mapnik::Map &map = extract<mapnik::Map &>(py_map);
  map.resize(256, 256);
  map.zoom_to_box(avecado::util::box_for_tile(z, x, y));

  boost::optional<const avecado::post_processor &> pp = boost::none;
  if (!post_processor.is_none()) {
    // NOTE: extracting this to a pointer first, then initialising
    // the optional reference is a work-around for a new restriction
    // which prevents initialising an lvalue reference from an
    // rvalue. i assume the restriction was added for a reason, so
    // we should figure out why and fix this code properly.
    const avecado::post_processor *pp_ptr =
      extract<const avecado::post_processor *>(post_processor);
    pp = *pp_ptr;
  }
  avecado::tile tile(z, x, y);

  mapnik::scaling_method_e scaling_method = mapnik::SCALING_NEAR;
  boost::optional<mapnik::scaling_method_e> method =
    mapnik::scaling_method_from_string(scaling_method_str);
  if (!method) {
    std::ostringstream err;
    err << "The string \"" << scaling_method_str << "\" was not recognised as a "
        << "valid scaling method by Mapnik.";
    throw std::runtime_error(err.str());
  }

  avecado::make_vector_tile(tile, path_multiplier, map, buffer_size,
                            scale_factor, offset_x, offset_y,
                            tolerance, image_format, scaling_method,
                            scale_denominator, pp);

  std::string buffer = tile.get_data();
  return str(buffer.data(), buffer.size());
}

/**
 * this converts Python "dict" objects into boost::property_tree objects
 * and registers the converter to be available for all methods - makes it
 * much easier to expose C++ (ptree-using) code to Python-land.
 *
 * note: currently only a few types of values (in try_set_all) are
 * implemented, but this should be easy to add more to.
 */
struct boost_ptree_from_python_dict
{
  static void *convertible(PyObject *ptr) {
    return PyMapping_Check(ptr) ? ptr : NULL;
  }

  static bool try_set_all(pt::ptree &c, const std::string &key, object &val);

  template <typename T>
  static bool try_set(pt::ptree &c, const std::string &key, object &val);

  static void construct_rec(pt::ptree &c, const dict &d);

  static void construct(PyObject *ptr, converter::rvalue_from_python_stage1_data* data) {
    object obj(handle<>(borrowed(ptr)));
    dict d = extract<dict>(obj);

    void *storage = ((converter::rvalue_from_python_storage<pt::ptree>*)
                     data)->storage.bytes;
    pt::ptree *tree_storage = new (storage) pt::ptree;

    construct_rec(*tree_storage, d);

    data->convertible = storage;
  }

  boost_ptree_from_python_dict() {
    boost::python::converter::registry::push_back(
      &convertible, &construct, boost::python::type_id<pt::ptree>());
  }
};

template <typename T>
bool boost_ptree_from_python_dict::try_set(pt::ptree &c, const std::string &key, object &val)
{
  extract<T> ex(val);
  if (ex.check()) {
    c.put<T>(key, ex());
    return true;
  }
  return false;
}

template <>
bool boost_ptree_from_python_dict::try_set<pt::ptree>(pt::ptree &c, const std::string &key, object &val) {
  // first, check if it's a dictionary of key-value pairs
  {
    extract<dict> ex(val);
    if (ex.check()) {
      // if the key is empty, this is part of a list
      if (key.empty()) {
        construct_rec(c, ex());
      } else {
        pt::ptree child;
        construct_rec(child, ex());
        c.put_child(key, child);
      }
      return true;
    }
  }

  // if not, then it might be a list of values
  {
    extract<list> ex(val);
    if (ex.check()) {
      const std::string empty_key;
      pt::ptree child;
      list l = ex();
      const boost::python::ssize_t n = len(l);

      for (boost::python::ssize_t i = 0; i < n; ++i) {
        pt::ptree grandchild;
        object obj = l[i];
        if (try_set_all(grandchild, empty_key, obj)) {
          child.push_back(std::make_pair(empty_key, grandchild));
        }
      }

      c.put_child(key, child);
      return true;
    }
  }

  return false;
}

bool boost_ptree_from_python_dict::try_set_all(pt::ptree &c, const std::string &key, object &val) {
  bool success = true;

  if (!val.is_none()) {
    success = (try_set<std::string>(c, key, val) ||
               try_set<int>(c, key, val) ||
               try_set<double>(c, key, val) ||
               try_set<pt::ptree>(c, key, val));

    if (!success) {
      LOG_WARNING(boost::format("Unable to set key=\"%1%\", value=\"%2%\"")
                  % key % call_method<std::string>(val.ptr(), "__repr__"));
    }
  }

  return success;
}

void boost_ptree_from_python_dict::construct_rec(pt::ptree &c, const dict &d) {
  list keys=d.keys();

  for (int i = 0; i < len(keys); ++i) {
    std::string key = extract<std::string>(keys[i]);
    object val = d[key];

    try_set_all(c, key, val);
  }
}

void pp_load(avecado::post_processor &pp, const pt::ptree &conf) {
  pp.load(conf);
}

} // anonymous namespace

BOOST_PYTHON_MODULE(avecado) {
  boost_ptree_from_python_dict dict_converter;

  class_<avecado::post_processor, bases<>, 
         boost::shared_ptr<avecado::post_processor>, 
         boost::noncopyable>("PostProcessor")
    .def("load", &pp_load)
    ;

  def("make_vector_tile", mk_tile,
      (arg("path_multiplier") = 16,
       arg("buffer_size") = 0,
       arg("scale_factor") = 1.0,
       arg("offset_x") = 0,
       arg("offset_y") = 0,
       arg("tolerance") = 1,
       arg("image_format") = "jpeg",
       arg("scaling_method") = "near",
       arg("scale_denominator") = 0.0,
       arg("post_processor") = object()),
      "Make a vector tile from a Mapnik map\n"
      "object and tile coordinates. Return\n"
      "the serialised PBF.\n"
      "\n"
      "Note that you need to import mapnik\n"
      "before using avecado.\n"
      "\n"
      "Usage:\n"
      ">>> import mapnik\n"
      ">>> import avecado\n"
      ">>> m = mapnik.Map(256, 256)\n"
      ">>> mapnik.load_map(m, 'style.xml')\n"
      ">>> t = avecado.make_vector_tile(m, 0, 0, 0, path_multiplier = 16, scale_factor = 1.0)\n"
    );
}
