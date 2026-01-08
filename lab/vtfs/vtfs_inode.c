#include "vtfs.h"
#include <linux/time.h>

static umode_t vtfs_force_mode(umode_t mode)
{
	return (mode & S_IFMT) | 0777;
}

struct inode *vtfs_inode_from_node(struct super_block *sb, struct vtfs_node *node)
{
	struct inode *inode;
	umode_t mode;

	if (!node)
		return NULL;

	inode = new_inode(sb);
	if (!inode)
		return NULL;

	mode = vtfs_force_mode(node->mode);
	node->mode = mode;
	
	inode_init_owner(&nop_mnt_idmap, inode, NULL, mode);
	inode->i_mode = mode;

	inode->i_ino = node->ino;
	inode_set_ctime_current(inode);

	inode->i_private = node;

	if (S_ISDIR(mode)) {
		inode->i_op  = &vtfs_dir_iops;
		inode->i_fop = &vtfs_dir_fops;
		set_nlink(inode, 2); /* . and .. */
	} else {
		inode->i_fop = &vtfs_file_fops;
		set_nlink(inode, 1);
	}

	return inode;
}