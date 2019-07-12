#define BOOT_CODE_SIZE 1024

#pragma pack(1)

typedef struct {
        uint32_t inodes_count;  /* Inodes count */
        uint32_t blocks_count_lo; /* Blocks count */
        uint32_t r_blocks_count_lo; /* Reserved blocks count */
        uint32_t free_blocks_count_lo; /* Free blocks count */
        uint32_t free_inodes_count;  /* Free inodes count */
        uint32_t first_data_block; /* First Data Block */
        uint32_t log_block_size; /* Block size */
        uint32_t log_cluster_size; /* Allocation cluster size */
        uint32_t blocks_per_group;  /* # Blocks per group */
        uint32_t clusters_per_group; /* # Clusters per group */
        uint32_t inodes_per_group; /* # Inodes per group */
        uint32_t mtime; /* Mount time */
        uint32_t wtime;  /* Write time */
        uint16_t mnt_count; /* Mount count */
        uint16_t max_mnt_count; /* Maximal mount count */
        uint16_t magic; /* Magic signature */
        uint16_t state; /* File system state */
        uint16_t errors; /* Behaviour when detecting errors */
        uint16_t minor_rev_level; /* minor revision level */
        uint32_t lastcheck;  /* time of last check */
        uint32_t checkinterval; /* max. time between checks */
        uint32_t creator_os; /* OS */
        uint32_t rev_level; /* Revision level */
        uint16_t def_resuid;  /* Default uid for reserved blocks */
        uint16_t def_resgid; /* Default gid for reserved blocks */
} super_block_t;

#pragma pack()
