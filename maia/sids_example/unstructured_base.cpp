#include "maia/sids_example/unstructured_base.hpp"
#include "maia/generate/from_structured_grid.hpp"
#include "maia/generate/ngons/from_homogenous_faces.hpp"
#include "maia/connectivity/utility.hpp"
#include "cpp_cgns/node_manip.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/repeat_n.hpp"
#include "maia/cpp_cgns_utils.hpp"
using namespace cgns;


auto
create_GridCoords0(factory F) -> tree {
  auto coord_X = make_cgns_vector(
    { 0.,1.,2.,3.,
      0.,1.,2.,3.,
      0.,1.,2.,3.,
      0.,1.,2.,3.,
      0.,1.,2.,3.,
      0.,1.,2.,3. }
    ,F.alloc()
  );
  auto coord_Y = make_cgns_vector(
    { 0.,0.,0.,0.,
      1.,1.,1.,1.,
      2.,2.,2.,2.,
      0.,0.,0.,0.,
      1.,1.,1.,1.,
      2.,2.,2.,2. }
    ,F.alloc()
  );
  auto coord_Z = make_cgns_vector(
    { 0.,0.,0.,0.,
      0.,0.,0.,0.,
      0.,0.,0.,0.,
      1.,1.,1.,1.,
      1.,1.,1.,1.,
      1.,1.,1.,1. }
    ,F.alloc()
  );
  tree grid_coords = F.newGridCoordinates();
  emplace_child(grid_coords,F.newDataArray("CoordinateX",view_as_node_value(coord_X)));
  emplace_child(grid_coords,F.newDataArray("CoordinateY",view_as_node_value(coord_Y)));
  emplace_child(grid_coords,F.newDataArray("CoordinateZ",view_as_node_value(coord_Z)));
  return grid_coords;
}
auto
create_GridCoords1(factory F) -> tree {
  auto coord_X = make_cgns_vector(
    { 3.,4.,
      3.,4.,
      3.,4.,
      3.,4. }
    ,F.alloc()
  );
  auto coord_Y = make_cgns_vector(
    { 0.,0.,
      1.,1.,
      0.,0.,
      1.,1. }
    ,F.alloc()
  );
  auto coord_Z = make_cgns_vector(
    { 0.,0.,
      0.,0.,
      1.,1.,
      1.,1. }
    ,F.alloc()
  );
  tree grid_coords = F.newGridCoordinates();
  emplace_child(grid_coords,F.newDataArray("CoordinateX",view_as_node_value(coord_X)));
  emplace_child(grid_coords,F.newDataArray("CoordinateY",view_as_node_value(coord_Y)));
  emplace_child(grid_coords,F.newDataArray("CoordinateZ",view_as_node_value(coord_Z)));
  return grid_coords;
}




auto
create_Zone0(factory F) -> tree {
/* Mesh used: "six quads", cf. simple_meshes.txt */
  int32_t VertexSize = 24;
  int32_t CellSize = 6;
  int32_t VertexSizeBoundary = 0;
  tree zone = F.newUnstructuredZone("Zone0",{VertexSize,CellSize,VertexSizeBoundary});

  emplace_child(zone,create_GridCoords0(F));

  auto pl_bc = make_cgns_vector<I4>(
    {1,2}, // 1,2 are the two i-faces at x=0
    F.alloc()
  );
  tree zone_bc = F.newZoneBC();
  emplace_child(zone_bc,F.newBC("Inlet","FaceCenter",pl_bc));
  emplace_child(zone,std::move(zone_bc));

  auto pl_gc = make_cgns_vector<I4>(
    {7}, // 7 is the bottom i-face at x=3
    F.alloc()
  );
  auto pl_gc_opp = make_cgns_vector<I4>(
    {1}, // cf. zone 1
    F.alloc()
  );
  tree gc = F.newGridConnectivity("MixingPlane","Zone1","FaceCenter","Abutting1to1");
  emplace_child(gc,F.newPointList("PointList",pl_gc));
  emplace_child(gc,F.newPointList("PointListDonor",pl_gc_opp));
  tree zone_gc = F.newZoneGridConnectivity();
  emplace_child(zone_gc,std::move(gc));
  emplace_child(zone,std::move(zone_gc));


  std_e::multi_index<int32_t,3> vertex_dims = {4,3,2};
  //auto quad_faces = generate_faces(vertex_dims) | ranges::to<std::vector>;
  std::vector<quad_4<int32_t>> quad_faces = generate_faces(vertex_dims) | ranges::to<std::vector>;
  maia::offset_vertices_ids(quad_faces,1); // CGNS is 1-indexed
  auto ngons = convert_to_interleaved_ngons(quad_faces) | to_cgns_vector(F.alloc());

  I8 nb_i_faces = 8;
  I8 nb_j_faces = 9;
  I8 nb_k_faces = 12;
  int32_t nb_ngons = nb_i_faces + nb_j_faces + nb_k_faces;

  auto i_faces_l_parent_elements = generate_faces_left_parent_cell_ids (vertex_dims,0);
  auto i_faces_r_parent_elements = generate_faces_right_parent_cell_ids(vertex_dims,0);
  auto j_faces_l_parent_elements = generate_faces_left_parent_cell_ids (vertex_dims,1);
  auto j_faces_r_parent_elements = generate_faces_right_parent_cell_ids(vertex_dims,1);
  // k-faces are considered interior (only for the sake of having interior nodes),
  // their parent is imaginary cell #42
  auto k_faces_l_parent_elements = ranges::views::repeat_n(42,nb_k_faces);
  auto k_faces_r_parent_elements = ranges::views::repeat_n(42,nb_k_faces);

  auto parent_elements = ranges::views::concat(
    i_faces_l_parent_elements , j_faces_l_parent_elements, k_faces_l_parent_elements,
    i_faces_r_parent_elements , j_faces_r_parent_elements, k_faces_r_parent_elements
  ) | to_cgns_vector(F.alloc());

  std_e::offset(parent_elements,1); // cgns indexing begin at 1
  std_e::multi_index<int,2> pe_dims = {(int)parent_elements.size()/2,2};
  auto parent_elts = view_as_md_array(parent_elements.data(),pe_dims);
  
  tree ngon_elts = F.newNgonElements(
    "Ngons",
    std_e::make_span(ngons),
    1,nb_ngons
  );
  emplace_child(ngon_elts,F.newDataArray("ParentElements", view_as_node_value(parent_elts)));
  emplace_child(zone,std::move(ngon_elts));

  return zone;
}

auto
create_Zone1(factory F) -> tree {
/* Le maillage utilisé est "one quad", cf. simple_meshes.h */
  int32_t VertexSize = 8;
  int32_t CellSize = 1;
  int32_t VertexSizeBoundary = 0;
  tree zone = F.newUnstructuredZone("Zone1",{VertexSize,CellSize,VertexSizeBoundary});

  emplace_child(zone,create_GridCoords1(F));

  auto pl_bc = make_cgns_vector<I4>(
    {2}, // 2 is the i-face at x=4
    F.alloc()
  );
  tree zone_bc = F.newZoneBC();
  emplace_child(zone_bc,F.newBC("Outlet","FaceCenter",pl_bc));
  emplace_child(zone,std::move(zone_bc));


  auto pl_gc = make_cgns_vector<I4>(
    {1}, // cf. zone 0
    F.alloc()
  );
  auto pl_gc_opp = make_cgns_vector<I4>(
    {7}, // 1 is the i-face at x=3
    F.alloc()
  );
  tree gc = F.newGridConnectivity("MixingPlane","Zone0","FaceCenter","Abutting1to1");
  emplace_child(gc,F.newPointList("PointList",pl_gc));
  emplace_child(gc,F.newPointList("PointListDonor",pl_gc_opp));
  tree zone_gc = F.newZoneGridConnectivity();
  emplace_child(zone_gc,std::move(gc));
  emplace_child(zone,std::move(zone_gc));


  std_e::multi_index<int32_t,3> vertex_dims = {2,2,2};
  auto quad_faces = generate_faces(vertex_dims) | ranges::to<std::vector>;
  maia::offset_vertices_ids(quad_faces,1); // CGNS is 1-indexed
  auto ngons = convert_to_interleaved_ngons(quad_faces) | to_cgns_vector(F.alloc());


  int32_t nb_ngons = 2 + 2 + 2;

  auto parent_elements = generate_faces_parent_cell_ids(vertex_dims) | to_cgns_vector(F.alloc());

  std_e::offset(parent_elements,1); // cgns indexing begin at 1
  std_e::multi_index<int,2> pe_dims = {(int)parent_elements.size()/2,2};
  auto parent_elts = view_as_md_array(parent_elements.data(),pe_dims);

  tree ngon_elts = F.newNgonElements(
    "Ngons",
    std_e::make_span(ngons),
    1,nb_ngons
  );
  emplace_child(ngon_elts,F.newDataArray("ParentElements", view_as_node_value(parent_elts)));
  emplace_child(zone,std::move(ngon_elts));

  return zone;
}

auto
create_unstructured_base(factory F) -> cgns::tree {
  tree b = F.newCGNSBase("Base0",3,3);
  emplace_child(b,create_Zone0(F));
  emplace_child(b,create_Zone1(F));
  return b;
}

