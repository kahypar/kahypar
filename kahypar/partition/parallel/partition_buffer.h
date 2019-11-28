#pragma once


namespace kahypar {
  struct BufferElement{
    int* partition;
    MPI_Request* request;
  
  };
class PartitionBuffer {

 
 

  static constexpr bool debug = true;
 public: 

  PartitionBuffer() :
    _partition_buffer(),
    _request_buffer() {}
  
  
  PartitionBuffer(const PartitionBuffer&) = delete;
  PartitionBuffer& operator= (const PartitionBuffer&) = delete;

  PartitionBuffer(PartitionBuffer&&) = delete;
  PartitionBuffer& operator= (PartitionBuffer&&) = delete;
  
  ~PartitionBuffer() {
    for( unsigned i = 0; i < _request_buffer.size(); i++) {
        MPI_Cancel( _request_buffer[i] );
    }
        
    for( unsigned i = 0; i < _request_buffer.size(); i++) {
      MPI_Status st;
      MPI_Wait( _request_buffer[i], & st );
      delete[] _partition_buffer[i];
      delete   _request_buffer[i];
    }
  }
  int size() {
    return _request_buffer.size();
  }
  BufferElement acquireBuffer(const std::vector<PartitionID>& partition_vector) {
      int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      int* partition_map = new int[partition_vector.size()];

      for(size_t i = 0; i < partition_vector.size(); ++i) {
        partition_map[i] = partition_vector[i];
      }
      
      MPI_Request* request = new MPI_Request();

    
    _partition_buffer.push_back(partition_map);
    _request_buffer.push_back(request);  
    return BufferElement {partition_map, request};
  }

  int* getData() {
    return _partition_buffer[0];
  }
  MPI_Request* getRequest() {
    return _request_buffer[0];
  }
  

  void releaseBuffer() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    LOG << preface() << "Partition Buffer has " << size() << " elements left.";
    for(unsigned i = 0; i < _request_buffer.size();) {
      int finished = 0;
      MPI_Status status;
      MPI_Test(_request_buffer[i], &finished, &status);
      if(finished) {
        LOG  <<preface() << "\x1B[32m" << "Partition Buffer request at position " << i << " is finished.\033[0m";
        std::swap(_request_buffer[i], _request_buffer[_request_buffer.size()-1]);
        std::swap(_partition_buffer[i], _partition_buffer[_request_buffer.size()-1]);

        delete[] _partition_buffer[_partition_buffer.size() - 1];
        delete   _request_buffer[_request_buffer.size() - 1];

        _partition_buffer.pop_back();
        _request_buffer.pop_back();
        LOG << preface() << "Partition Buffer has " << size() << " elements left.";
      }
      else {
        
        LOG  << preface() << "\x1B[31m" << "Partition Buffer request at position " << i << " is NOT finished.\033[0m";
        ++i;
      }
    }
  }

 private:
 inline std::string preface() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   return "[MPI Rank " + std::to_string(rank) + "] ";
 }
 std::vector<int*> _partition_buffer;
 std::vector<MPI_Request*> _request_buffer;
};



} //namespace kahypar

