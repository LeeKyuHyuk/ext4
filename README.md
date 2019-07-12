EXT4 Data Structures and Algorithms
-----------------------------------

# 1. About this Book
이 문서는 ext4 파일 시스템에 대한 내용을 설명하려고합니다. ext2가 지원하는 모든 기능을 지원하지는 않지만 동일하게 ext2,ext3 파일 시스템에도 적용되어있으며 필드는 더 짧을 것입니다.

## 1.1. License
이 글은 GNU Public License, v2 라이센스 입니다.

## 1.2. Terminology
EXT4는 저장 장치를 Logical Block으로 나누어 오버헤드를 줄이고 더 큰 전송 크기를 통하여 처리량을 늘렸습니다. 일반적으로 실제 크기는 `2 ^ (10 + sb.s_log_block_size)` Byte로 계산되지만 Block 크기는 4KiB (x86에서 페이지와 동일한 크기 및 Block 계층의 기본 Block 크기)가 됩니다. 이 문서에서 디스크 위치는 Raw LBA(1024 Byte Block)이 아닌 Logical Block로 표기됩니다. 편의상 Logical Block 크기는 `$block_size`라고 표기합니다.

# 2. High Level Design
EXT4 파일시스템은 일련의 Block Group으로 분할됩니다. Block Group의 크기는 `sb.s_blocks_per_group`에 지정되어 있지만, `8 * block_size_in_bytes`로 계산할 수도 있습니다. 기본 Block 크기가 4KiB이고 128MiB인 경우에는 32,768Block(131,072KiB / 4KiB = 32,768)을 가지게 됩니다. Block Group의 개수는 장치의 크기를 Block Group의 크기로 나눈 것 입니다.  
EXT4의 모든 필드는 Little-Endian 순서로 디스크에 기록됩니다. 그러나, JBD2(Journal)의 모든 필드는 Big-endian 순서로 디스크에 기록됩니다.

# 2.1. Blocks
EXT4는 "Block" 단위로 저장 공간을 할당합니다. Block은 1KiB에서 64KiB 사이의 섹터 그룹이며 섹터 수는 2의 정수 배수여야 합니다. Block는 Block Group라는 더 큰 단위로 차례로 묶여지게 됩니다.

# 2.2. Layout
표준 Block Group의 Layout은 다음과 같습니다.

| Group 0 Padding | ext4 Super Block | Group Descriptors | Reserved GDT Blocks | Data Block Bitmap | inode Bitmap | inode Table | Data Blocks      |
|-----------------|------------------|-------------------|---------------------|-------------------|--------------|-------------|------------------|
| 1024 bytes      | 1 block          | many blocks       | many blocks         | 1 block           | 1 block      | many blocks | many more blocks |

Block Group 0는 특수한 케이스입니다. x86 부트 섹터 및 기타 등등을 고려하여 처음 1024 Byte가 사용되지 않습니다. SuperBlock은 어느 블록이든간에 Offset 1024 Byte에서 시작합니다.

inode table의 위치는 `grp.bg_inode_table_*.`에 의해 주어집니다. `sb.s_inodes_per_group * sb.s_inode_size` Byte를 포함 할 수 있을만큼 커다란 Block 입니다.
