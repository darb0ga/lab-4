#include "vtfs.h"
#include <linux/pagemap.h>
#include <linux/printk.h>

static struct dentry *vtfs_mount(struct file_system_type *fs_type,
                                 int flags,
                                 const char *dev_name,
                                 void *data)
{
	pr_info("[vtfs] mount request\n");
	return mount_nodev(fs_type, flags, data, vtfs_fill_super);
}

static void vtfs_kill_sb(struct super_block *sb)
{
	pr_info("[vtfs] kill_sb\n");
	kill_litter_super(sb);
}

struct file_system_type vtfs_fs_type = {
	.owner   = THIS_MODULE,
	.name    = "vtfs",
	.mount   = vtfs_mount,
	.kill_sb = vtfs_kill_sb,
	.fs_flags = FS_USERNS_MOUNT,
};

static void vtfs_put_super(struct super_block *sb)
{
	vtfs_store_destroy(sb);
}

const struct super_operations vtfs_super_ops = {
	.put_super  = vtfs_put_super,
	.statfs     = simple_statfs,
	.drop_inode = generic_delete_inode,
};

int vtfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct vtfs_node *root_node;
	int err;

	(void)data;
	(void)silent;

	sb->s_magic = 0x76746673; /* "vtfs" */
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_op = &vtfs_super_ops;
	sb->s_time_gran = 1;

	err = vtfs_store_init(sb);
	if (err)
		return err;

	root_node = vtfs_store_root(sb);
	if (!root_node)
		return -ENOMEM;

	root_inode = vtfs_inode_from_node(sb, root_node);
	if (!root_inode)
		return -ENOMEM;

	sb->s_root = d_make_root(root_inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}