#include <rdmacred.h>
#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define EPRINTF(etag, ...)                              \
{                                                       \
    int erank = 0;                                      \
    MPI_Comm_rank(MPI_COMM_WORLD, &erank);              \
    char etmpbuf1[1024] = {'\0'};                       \
    char etmpbuf2[1024] = {'\0'};                       \
    char etmpbuf3[1024] = {'\0'};                       \
    snprintf(etmpbuf1, 1023, etag " : [%d][%s][%d]\n",  \
        erank, __FILE__, __LINE__);                     \
    snprintf(etmpbuf2, 1023, __VA_ARGS__ );             \
    snprintf(etmpbuf3, 1023, etag " : %s \n", etmpbuf2);\
    fprintf(stderr, etmpbuf1);                          \
    fprintf(stderr, etmpbuf3);                          \
}

#define RDMA_DP_ERROR(...)                              \
    EPRINTF("ERROR", __VA_ARGS__)

#define RDMA_DP_VERBOSE
#if defined(RDMA_DP_VERBOSE)
#define RDMA_DP_STATUS(...)                             \
    EPRINTF("STATUS", __VA_ARGS__)
#else
#define RDMA_DP_STATUS(...) {}
#endif

const char *drc_strerror(int code)
{
    const char *str = "unknown error";
    switch (-code)
    {
        case DRC_SUCCESS:
            str = "DRC_SUCCESS ( success )";
            break;
        case DRC_EINVAL:
            str = "DRC_EINVAL ( invalid parameter )";
            break;
        case DRC_EPERM:
            str = "DRC_EPERM ( invalid permissions )";
            break;
        case DRC_ENOSPC:
            str = "DRC_ENOSPC ( no space left )";
            break;
        case DRC_ECONNREFUSED:
            str = "DRC_ECONNREFUSED ( connection refused )";
            break;
        case DRC_ALREADY_GRANTED:
            str = "DRC_ALREADY_GRANTED ( access already granted )";
            break;
        case DRC_CRED_NOT_FOUND:
            str = "DRC_CRED_NOT_FOUND ( credential not found )";
            break;
        case DRC_CRED_CREATE_FAILURE:
            str = "DRC_CRED_CREATE_FAILURE ( could not create credential )";
            break;
        case DRC_CRED_EXTERNAL_FAILURE:
            str = "DRC_CRED_EXTERNAL_FAILURE ( external application failure )";
            break;
        case DRC_BAD_TOKEN:
            str = "DRC_BAD_TOKEN ( bad/expired token passed to system )";
            break;
    }
    return str;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;

    int rank = 0;
    int n_ranks = 1;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &n_ranks);

    int rc = 0;
    uint32_t credential = 0;
    drc_info_handle_t drc_info;

    if (rank == 0)
    {
        rc = drc_acquire(&credential, DRC_FLAGS_FLEX_CREDENTIAL);
        if (rc != DRC_SUCCESS)
        {
            RDMA_DP_ERROR("Could not acquire DRC credential. " "Failed with %d.", rc);
            RDMA_DP_ERROR("drc_acquire failed! %d : %s", rc, drc_strerror(rc))
            MPI_Abort(comm, -1);
        }
        else
        {
            RDMA_DP_STATUS("DRC acquired credential id %d", credential)
        }
    }

    // NOTE: credential is a uint32_t.
    MPI_Bcast(&credential, sizeof(credential), MPI_BYTE, 0, comm);


    int try = 0;
    int max_try = 120;
    do
    {
        sleep(1);
        rc = drc_access(credential, 0, &drc_info);
        try += 1;
    }
    while ((rc != DRC_SUCCESS) && (try < max_try));

    if (rc != DRC_SUCCESS)
    {
        RDMA_DP_ERROR("drc_access %u failed! %d : %s", credential,
            rc, drc_strerror(rc))
        MPI_Abort(comm, -1);
    }

    //RDMA_DP_STATUS("DRC accessed credential id %d", credential)

    rc = drc_release(credential, 0);

    // print the distribution of attempts beffore success
    int *all_try = (int*)malloc(n_ranks*sizeof(int));
    MPI_Gather(&try, 1, MPI_INT, all_try, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        size_t n_bytes = max_try*sizeof(int);
        int *try_hist = malloc(n_bytes);
        memset(try_hist, 0, n_bytes);
        for (int i = 0; i < n_ranks; ++i)
            try_hist[all_try[i]] += 1;
        fprintf(stderr, "pass : number of ranks succeeded\n");
        for (int i = 0; i < max_try; ++i)
            fprintf(stderr, "%d : %d\n", i, try_hist[i]);
    }

    MPI_Finalize();

    return 0;
}
