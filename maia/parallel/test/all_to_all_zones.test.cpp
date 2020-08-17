#include "maia/parallel/all_to_all_zones.hpp"

#include "std_e/unit_test/mpi/doctest.hpp"

using namespace std;



MPI_TEST_CASE("all_to_all",3) {
  using data_t = std::vector<int>;
  std::vector<data_t> data_to_send(3);
  if (std_e::rank(test_comm)==0) {
    data_to_send[0] = {1,2};
    data_to_send[1] = {3,4,5};
    data_to_send[2] = {6,7};
  else if (std_e::rank(test_comm)==1) {
    data_to_send[0] = {8};
    data_to_send[1] = {9,10,11,12};
    data_to_send[2] = {13};
  } else {
    STD_E_ASSERT(std_e::rank(test_comm)==2) {
    data_to_send[0] = {};
    data_to_send[1] = {14};
    data_to_send[2] = {15,16};
  }

  std::vector<data_t> data_received = all_to_all(data_to_send,test_comm);

  MPI_CHECK(0, data_received[0] == vector{1,2} );
  MPI_CHECK(0, data_received[1] == vector{8} );
  MPI_CHECK(0, data_received[2] == vector{} );

  MPI_CHECK(1, data_received[0] == vector{3,4,5} );
  MPI_CHECK(1, data_received[1] == vector{9,10,11,12} );
  MPI_CHECK(1, data_received[2] == vector{14} );

  MPI_CHECK(2, data_received[0] == vector{6,7} );
  MPI_CHECK(2, data_received[1] == vector{13} );
  MPI_CHECK(2, data_received[2] == vector{15,16} );
}
