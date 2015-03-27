#include <stdlib.h>
#include <hdf5.h>

#include "ASDF_read.h"
#include "ASDF_common.h"

hid_t ASDF_open_read_only(char *filename, MPI_Comm comm) {
   hid_t plist_id, file_id;

   CHK_H5(plist_id = H5Pcreate (H5P_FILE_ACCESS));
   CHK_H5(H5Pset_fapl_mpio(plist_id, comm, MPI_INFO_NULL));
   CHK_H5(file_id = H5Fopen (filename, H5F_ACC_RDONLY, plist_id));
   CHK_H5(H5Pclose(plist_id));

   return file_id;
}

int ASDF_read_str_attr(hid_t file_id, char *grp_name,
                       char *attr_name, char **attr_value) {
    hid_t attr_id, type, memtype;
    size_t size;
    int success = 0;

    attr_id = H5Aopen_by_name(file_id, grp_name, attr_name, H5P_DEFAULT, H5P_DEFAULT);
    type = H5Aget_type(attr_id);
    size = H5Tget_size(type);
    size++;  // make a space for null terminator
    *attr_value= (char *) malloc(size* sizeof(char));
    type = H5Tcopy(H5T_C_S1);
    H5Tset_size(type, size);
    if (H5Aread(attr_id, type, *attr_value) >= 0)
        success = -1;
    else
        free(*attr_value);
    H5Tclose(type);
    H5Aclose(attr_id);
    if (!success)
        *attr_value= NULL;
    return success;
}

int ASDF_get_num_elements_dataset(hid_t dataset_id) {
    hsize_t memsize;
    hid_t dataset_type;
    size_t type_size;

    memsize = H5Dget_storage_size(dataset_id);
    dataset_type = H5Dget_type(dataset_id);
    type_size = H5Tget_size(dataset_type);
    
    return (int) memsize / type_size;
}

int ASDF_get_num_elements_from_path(hid_t file_id, char *path) {
    hid_t dataset_id;
    int num_elems;

    dataset_id = H5Dopen(file_id, path, H5P_DEFAULT);
    num_elems = ASDF_get_num_elements_dataset(dataset_id);

    H5Dclose(dataset_id);
    return num_elems;
}

int ASDF_read_full_waveform(hid_t file_id, char *path, float *waveform) {
    hid_t dataset_id;
    dataset_id = H5Dopen(file_id, path, H5P_DEFAULT);

    H5Dread(dataset_id, H5T_IEEE_F32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, waveform);
    H5Dclose(dataset_id);

    return 0;
}

int ASDF_read_partial_waveform(hid_t file_id, char *path, int offset,
                               int nsamples, float *waveform) {
  hid_t dataset_id;
  hid_t space_id, slab_id;

  hsize_t start[1] = {offset};
  hsize_t count[1] = {1};
  hsize_t block[1] = {nsamples};

  dataset_id = H5Dopen(file_id, path, H5P_DEFAULT);
  CHK_H5(space_id = H5Dget_space(dataset_id));
  CHK_H5(H5Sselect_hyperslab(space_id, H5S_SELECT_SET, start, 
                             NULL, count, block));
  H5Dread(dataset_id, H5T_IEEE_F32LE, H5S_ALL, H5S_ALL,
          H5P_DEFAULT, waveform);
  CHK_H5(H5Sclose(space_id));
  CHK_H5(H5Dclose(dataset_id));

  return 0;
}