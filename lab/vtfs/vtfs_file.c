#include "vtfs.h"

const struct inode_operations vtfs_file_iops = {
	/* пока пусто */
};

const struct file_operations vtfs_file_fops = {
	.owner = THIS_MODULE,
	.llseek = generic_file_llseek,
};