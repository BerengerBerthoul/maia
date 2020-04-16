#include "doctest/doctest.h"

#include "maia/connectivity/connectivity_ref.hpp"
#include "maia/connectivity/test/test_utils.hpp"

using namespace std;

using con_view_type = connectivity_ref<int,my_connectivity_kind>;
using con_const_view_type = connectivity_ref<const int,my_connectivity_kind>;

TEST_CASE("connectivity_ref") {
  vector<int> vertices = {1,2,3};
  int* v_ptr = vertices.data();
  const int* v_const_ptr = vertices.data();



  con_const_view_type con_const_view(v_const_ptr);
  con_view_type       con_view_type      (v_ptr);


  SUBCASE("basic_tests") {
    CHECK( con_view_type.type   == my_connectivity_type_id );
    CHECK( con_view_type.size() == 3                       );
  }

  SUBCASE("equality") {
    vector<int> same_vertices = {1,2,3};
    con_const_view_type same(same_vertices.data());
    CHECK( con_view_type == same );

    vector<int> different_vertices = {2,3,1};
    con_const_view_type different(different_vertices.data());
    CHECK( con_view_type != different );
  }


  SUBCASE("read") {
    CHECK( con_const_view[0] == 1 );
    CHECK( con_const_view[1] == 2 );
    CHECK( con_const_view[2] == 3 );
  }
      
  SUBCASE("write") {
    con_view_type[0] = 10;
    con_view_type[1] = 11;
    con_view_type[2] = 12;

    CHECK( con_const_view[0] == 10 );
    CHECK( con_const_view[1] == 11 );
    CHECK( con_const_view[2] == 12 );
  }

  SUBCASE("begin_and_end") {
    CHECK( con_view_type.begin() == vertices.data()  );
    CHECK( con_view_type.end()   == vertices.data()+3);
  }
}
