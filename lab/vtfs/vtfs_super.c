#include "vtfs.h"

struct file_system_type vtfs_fs_type = {
	.owner   = THIS_MODULE,
	.name    = VTFS_NAME,
	.mount   = vtfs_mount,
	.kill_sb = vtfs_kill_sb,
};

struct dentry *vtfs_mount(struct file_system_type *fs_type,
                          int flags,
                          const char *token,
                          void *data)
{
	pr_info("[vtfs] mount request\n");
	return mount_nodev(fs_type, flags, data, vtfs_fill_super);
}

void vtfs_kill_sb(struct super_block *sb)
{
	pr_info("[vtfs] kill_sb\n");
	vtfs_store_destroy(sb);
	kill_litter_super(sb);
	pr_info("[vtfs] superblock destroyed\n");
}
