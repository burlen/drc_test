This is a simple program to reproduce drc eror we encountered on cori knl.

# build

```shell
module swap PrgEnv-intel PrgEnv-gnu
module load rdma-credentials
export CRAYPE_LINK_TYPE=dynamic
make
```

# run

```shell
sbatch submit_drc_test.sh
```
