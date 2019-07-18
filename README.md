EXT4 Data Structures and Algorithms
-----------------------------------

# 1. About this Book
This document attempts to describe the on-disk format for ext4 filesystems. The same general ideas should apply to ext2/3 filesystems as well, though they do not support all the features that ext4 supports, and the fields will be shorter.

**NOTE:** This is a work in progress, based on notes that the author (djwong) made while picking apart a filesystem by hand. The data structure definitions should be current as of Linux 4.18 and e2fsprogs-1.44. All comments and corrections are welcome, since there is undoubtedly plenty of lore that might not be reflected in freshly created demonstration filesystems.

## 1.1. License
This book is licensed under the terms of the GNU Public License, v2.

## 1.2. Terminology
ext4 divides a storage device into an array of logical blocks both to reduce bookkeeping overhead and to increase throughput by forcing larger transfer sizes. Generally, the block size will be 4KiB (the same size as pages on x86 and the block layer’s default block size), though the actual size is calculated as 2 ^ (10 + `sb.s_log_block_size`) bytes. Throughout this document, disk locations are given in terms of these logical blocks, not raw LBAs, and not 1024-byte blocks. For the sake of convenience, the logical block size will be referred to as `$block_size` throughout the rest of the document.

When referenced in `preformatted text` blocks, sb refers to fields in the super block, and inode refers to fields in an inode table entry.

## 1.3. Other References
Also see http://www.nongnu.org/ext2-doc/ for quite a collection of information about ext2/3. Here’s another old reference: http://wiki.osdev.org/Ext2

# 2. High Level Design
An ext4 file system is split into a series of block groups. To reduce performance difficulties due to fragmentation, the block allocator tries very hard to keep each file’s blocks within the same group, thereby reducing seek times. The size of a block group is specified in `sb.s_blocks_per_group` blocks, though it can also calculated as 8 * `block_size_in_bytes`. With the default block size of 4KiB, each group will contain 32,768 blocks, for a length of 128MiB. The number of block groups is the size of the device divided by the size of a block group.

All fields in ext4 are written to disk in little-endian order. HOWEVER, all fields in jbd2 (the journal) are written to disk in big-endian order.

## 2.1. Blocks
ext4 allocates storage space in units of “blocks”. A block is a group of sectors between 1KiB and 64KiB, and the number of sectors must be an integral power of 2. Blocks are in turn grouped into larger units called block groups. Block size is specified at mkfs time and typically is 4KiB. You may experience mounting problems if block size is greater than page size (i.e. 64KiB blocks on a i386 which only has 4KiB memory pages). By default a filesystem can contain 2^32 blocks; if the ‘64bit’ feature is enabled, then a filesystem can have 2^64 blocks.

For 32-bit filesystems, limits are as follows:

For 64-bit filesystems, limits are as follows:

Note: Files not using extents (i.e. files using block maps) must be placed within the first 2^32 blocks of a filesystem. Files with extents must be placed within the first 2^48 blocks of a filesystem. It’s not clear what happens with larger filesystems.

## 2.2. Layout
The layout of a standard block group is approximately as follows (each of these fields is discussed in a separate section below):

| Group 0 Padding | ext4 Super Block | Group Descriptors | Reserved GDT Blocks | Data Block Bitmap | inode Bitmap | inode Table | Data Blocks      |
|-----------------|------------------|-------------------|---------------------|-------------------|--------------|-------------|------------------|
| 1024 bytes      | 1 block          | many blocks       | many blocks         | 1 block           | 1 block      | many blocks | many more blocks |

For the special case of block group 0, the first 1024 bytes are unused, to allow for the installation of x86 boot sectors and other oddities. The superblock will start at offset 1024 bytes, whichever block that happens to be (usually 0). However, if for some reason the block size = 1024, then block 0 is marked in use and the superblock goes in block 1. For all other block groups, there is no padding.

The ext4 driver primarily works with the superblock and the group descriptors that are found in block group 0. Redundant copies of the superblock and group descriptors are written to some of the block groups across the disk in case the beginning of the disk gets trashed, though not all block groups necessarily host a redundant copy (see following paragraph for more details). If the group does not have a redundant copy, the block group begins with the data block bitmap. Note also that when the filesystem is freshly formatted, mkfs will allocate “reserve GDT block” space after the block group descriptors and before the start of the block bitmaps to allow for future expansion of the filesystem. By default, a filesystem is allowed to increase in size by a factor of 1024x over the original filesystem size.

The location of the inode table is given by `grp.bg_inode_table_*`. It is continuous range of blocks large enough to contain `sb.s_inodes_per_group * sb.s_inode_size` bytes.

As for the ordering of items in a block group, it is generally established that the super block and the group descriptor table, if present, will be at the beginning of the block group. The bitmaps and the inode table can be anywhere, and it is quite possible for the bitmaps to come after the inode table, or for both to be in different groups (flex_bg). Leftover space is used for file data blocks, indirect block maps, extent tree blocks, and extended attributes.

## 2.3. Flexible Block Groups
Starting in ext4, there is a new feature called flexible block groups (flex_bg). In a flex_bg, several block groups are tied together as one logical block group; the bitmap spaces and the inode table space in the first block group of the flex_bg are expanded to include the bitmaps and inode tables of all other block groups in the flex_bg. For example, if the flex_bg size is 4, then group 0 will contain (in order) the superblock, group descriptors, data block bitmaps for groups 0-3, inode bitmaps for groups 0-3, inode tables for groups 0-3, and the remaining space in group 0 is for file data. The effect of this is to group the block metadata close together for faster loading, and to enable large files to be continuous on disk. Backup copies of the superblock and group descriptors are always at the beginning of block groups, even if flex_bg is enabled. The number of block groups that make up a flex_bg is given by 2 ^ `sb.s_log_groups_per_flex`.

## 2.4. Meta Block Groups
Without the option META_BG, for safety concerns, all block group descriptors copies are kept in the first block group. Given the default 128MiB(2^27 bytes) block group size and 64-byte group descriptors, ext4 can have at most 2^27/64 = 2^21 block groups. This limits the entire filesystem size to 2^21 ∗ 2^27 = 2^48bytes or 256TiB.

The solution to this problem is to use the metablock group feature (META_BG), which is already in ext3 for all 2.6 releases. With the META_BG feature, ext4 filesystems are partitioned into many metablock groups. Each metablock group is a cluster of block groups whose group descriptor structures can be stored in a single disk block. For ext4 filesystems with 4 KB block size, a single metablock group partition includes 64 block groups, or 8 GiB of disk space. The metablock group feature moves the location of the group descriptors from the congested first block group of the whole filesystem into the first group of each metablock group itself. The backups are in the second and last group of each metablock group. This increases the 2^21 maximum block groups limit to the hard limit 2^32, allowing support for a 512PiB filesystem.

The change in the filesystem format replaces the current scheme where the superblock is followed by a variable-length set of block group descriptors. Instead, the superblock and a single block group descriptor block is placed at the beginning of the first, second, and last block groups in a meta-block group. A meta-block group is a collection of block groups which can be described by a single block group descriptor block. Since the size of the block group descriptor structure is 32 bytes, a meta-block group contains 32 block groups for filesystems with a 1KB block size, and 128 block groups for filesystems with a 4KB blocksize. Filesystems can either be created using this new block group descriptor layout, or existing filesystems can be resized on-line, and the field s_first_meta_bg in the superblock will indicate the first block group using this new layout.

Please see an important note about `BLOCK_UNINIT` in the section about block and inode bitmaps.

## 2.5. Lazy Block Group Initialization
A new feature for ext4 are three block group descriptor flags that enable mkfs to skip initializing other parts of the block group metadata. Specifically, the INODE_UNINIT and BLOCK_UNINIT flags mean that the inode and block bitmaps for that group can be calculated and therefore the on-disk bitmap blocks are not initialized. This is generally the case for an empty block group or a block group containing only fixed-location block group metadata. The INODE_ZEROED flag means that the inode table has been initialized; mkfs will unset this flag and rely on the kernel to initialize the inode tables in the background.

By not writing zeroes to the bitmaps and inode table, mkfs time is reduced considerably. Note the feature flag is RO_COMPAT_GDT_CSUM, but the dumpe2fs output prints this as “uninit_bg”. They are the same thing.

## 2.6. Special inodes
ext4 reserves some inode for special features, as follows:

| inode Number | Purpose |
|--------------|---------|
| 0 | Doesn’t exist; there is no inode 0. |
| 1 | List of defective blocks. |
| 2 | Root directory. |
| 3 | User quota. |
| 4 | Group quota. |
| 5 | Boot loader. |
| 6 | Undelete directory. |
| 7 | Reserved group descriptors inode. (“resize inode”) |
| 8 | Journal inode. |
| 9 | The “exclude” inode, for snapshots(?) |
| 10 | Replica inode, used for some non-upstream feature? |
| 11 | Traditional first non-reserved inode. Usually this is the lost+found directory. See s_first_ino in the superblock. |

## 2.7. Block and Inode Allocation Policy
ext4 recognizes (better than ext3, anyway) that data locality is generally a desirably quality of a filesystem. On a spinning disk, keeping related blocks near each other reduces the amount of movement that the head actuator and disk must perform to access a data block, thus speeding up disk IO. On an SSD there of course are no moving parts, but locality can increase the size of each transfer request while reducing the total number of requests. This locality may also have the effect of concentrating writes on a single erase block, which can speed up file rewrites significantly. Therefore, it is useful to reduce fragmentation whenever possible.

The first tool that ext4 uses to combat fragmentation is the multi-block allocator. When a file is first created, the block allocator speculatively allocates 8KiB of disk space to the file on the assumption that the space will get written soon. When the file is closed, the unused speculative allocations are of course freed, but if the speculation is correct (typically the case for full writes of small files) then the file data gets written out in a single multi-block extent. A second related trick that ext4 uses is delayed allocation. Under this scheme, when a file needs more blocks to absorb file writes, the filesystem defers deciding the exact placement on the disk until all the dirty buffers are being written out to disk. By not committing to a particular placement until it’s absolutely necessary (the commit timeout is hit, or sync() is called, or the kernel runs out of memory), the hope is that the filesystem can make better location decisions.

The third trick that ext4 (and ext3) uses is that it tries to keep a file’s data blocks in the same block group as its inode. This cuts down on the seek penalty when the filesystem first has to read a file’s inode to learn where the file’s data blocks live and then seek over to the file’s data blocks to begin I/O operations.

The fourth trick is that all the inodes in a directory are placed in the same block group as the directory, when feasible. The working assumption here is that all the files in a directory might be related, therefore it is useful to try to keep them all together.

The fifth trick is that the disk volume is cut up into 128MB block groups; these mini-containers are used as outlined above to try to maintain data locality. However, there is a deliberate quirk – when a directory is created in the root directory, the inode allocator scans the block groups and puts that directory into the least heavily loaded block group that it can find. This encourages directories to spread out over a disk; as the top-level directory/file blobs fill up one block group, the allocators simply move on to the next block group. Allegedly this scheme evens out the loading on the block groups, though the author suspects that the directories which are so unlucky as to land towards the end of a spinning drive get a raw deal performance-wise.

Of course if all of these mechanisms fail, one can always use e4defrag to defragment files.

## 2.8. Checksums
Starting in early 2012, metadata checksums were added to all major ext4 and jbd2 data structures. The associated feature flag is metadata_csum. The desired checksum algorithm is indicated in the superblock, though as of October 2012 the only supported algorithm is crc32c. Some data structures did not have space to fit a full 32-bit checksum, so only the lower 16 bits are stored. Enabling the 64bit feature increases the data structure size so that full 32-bit checksums can be stored for many data structures. However, existing 32-bit filesystems cannot be extended to enable 64bit mode, at least not without the experimental resize2fs patches to do so.

Existing filesystems can have checksumming added by running tune2fs -O metadata_csum against the underlying device. If tune2fs encounters directory blocks that lack sufficient empty space to add a checksum, it will request that you run e2fsck -D to have the directories rebuilt with checksums. This has the added benefit of removing slack space from the directory files and rebalancing the htree indexes. If you _ignore_ this step, your directories will not be protected by a checksum!

The following table describes the data elements that go into each type of checksum. The checksum function is whatever the superblock describes (crc32c as of October 2013) unless noted otherwise.

| Metadata | Length | Ingredients |
|---|---|---|
| Superblock | __le32 | The entire superblock up to the checksum field. The UUID lives inside the superblock. |
| MMP | __le32 | UUID + the entire MMP block up to the checksum field. |
| Extended Attributes | __le32 | UUID + the entire extended attribute block. The checksum field is set to zero. |
| Directory Entries | __le32 | UUID + inode number + inode generation + the directory block up to the fake entry enclosing the checksum field. |
| HTREE Nodes | __le32 | UUID + inode number + inode generation + all valid extents + HTREE tail. The checksum field is set to zero. |
| Extents | __le32 | UUID + inode number + inode generation + the entire extent block up to the checksum field. |
| Bitmaps | __le32 or __le16 | UUID + the entire bitmap. Checksums are stored in the group descriptor, and truncated if the group descriptor size is 32 bytes (i.e. ^64bit) |
| Inodes | __le32 | UUID + inode number + inode generation + the entire inode. The checksum field is set to zero. Each inode has its own checksum. |
| Group Descriptors | __le16 | If metadata_csum, then UUID + group number + the entire descriptor; else if gdt_csum, then crc16(UUID + group number + the entire descriptor). In all cases, only the lower 16 bits are stored. |

## 2.9. Bigalloc
At the moment, the default size of a block is 4KiB, which is a commonly supported page size on most MMU-capable hardware. This is fortunate, as ext4 code is not prepared to handle the case where the block size exceeds the page size. However, for a filesystem of mostly huge files, it is desirable to be able to allocate disk blocks in units of multiple blocks to reduce both fragmentation and metadata overhead. The bigalloc feature provides exactly this ability. The administrator can set a block cluster size at mkfs time (which is stored in the s_log_cluster_size field in the superblock); from then on, the block bitmaps track clusters, not individual blocks. This means that block groups can be several gigabytes in size (instead of just 128MiB); however, the minimum allocation unit becomes a cluster, not a block, even for directories. TaoBao had a patchset to extend the “use units of clusters instead of blocks” to the extent tree, though it is not clear where those patches went– they eventually morphed into “extent tree v2” but that code has not landed as of May 2015.

## 2.10. Inline Data
The inline data feature was designed to handle the case that a file’s data is so tiny that it readily fits inside the inode, which (theoretically) reduces disk block consumption and reduces seeks. If the file is smaller than 60 bytes, then the data are stored inline in inode.i_block. If the rest of the file would fit inside the extended attribute space, then it might be found as an extended attribute “system.data” within the inode body (“ibody EA”). This of course constrains the amount of extended attributes one can attach to an inode. If the data size increases beyond i_block + ibody EA, a regular block is allocated and the contents moved to that block.

Pending a change to compact the extended attribute key used to store inline data, one ought to be able to store 160 bytes of data in a 256-byte inode (as of June 2015, when i_extra_isize is 28). Prior to that, the limit was 156 bytes due to inefficient use of inode space.

The inline data feature requires the presence of an extended attribute for “system.data”, even if the attribute value is zero length.

### 2.10.1. Inline Directories
The first four bytes of i_block are the inode number of the parent directory. Following that is a 56-byte space for an array of directory entries; see struct `ext4_dir_entry`. If there is a “system.data” attribute in the inode body, the EA value is an array of struct `ext4_dir_entry` as well. Note that for inline directories, the i_block and EA space are treated as separate dirent blocks; directory entries cannot span the two.

Inline directory entries are not checksummed, as the inode checksum should protect all inline data contents.

## 2.11. Large Extended Attribute Values
To enable ext4 to store extended attribute values that do not fit in the inode or in the single extended attribute block attached to an inode, the EA_INODE feature allows us to store the value in the data blocks of a regular file inode. This “EA inode” is linked only from the extended attribute name index and must not appear in a directory entry. The inode’s i_atime field is used to store a checksum of the xattr value; and i_ctime/i_version store a 64-bit reference count, which enables sharing of large xattr values between multiple owning inodes. For backward compatibility with older versions of this feature, the i_mtime/i_generation *may* store a back-reference to the inode number and i_generation of the **one** owning inode (in cases where the EA inode is not referenced by multiple inodes) to verify that the EA inode is the correct one being accessed.
