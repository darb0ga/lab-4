#include "vtfs.h"
#include <linux/fs.h>
#include <linux/time.h>

/*
 * Create VFS inode from vtfs_node
 */
struct inode *vtfs_make_inode(struct super_block *sb,
                              struct vtfs_node *node)
{
	struct inode *inode;

	inode = new_inode(sb);
	if (!inode)
		return NULL;

	inode->i_ino = node->ino;
	inode->i_mode = node->mode;

	inode_init_owner(&nop_mnt_idmap, inode, NULL, node->mode);

	inode_set_ctime_current(inode);


	if (S_ISDIR(node->mode)) {
		inode->i_op  = &vtfs_inode_ops;
		inode->i_fop = &vtfs_dir_ops;
	} else {
		inode->i_op  = &vtfs_inode_ops;
		inode->i_fop = &vtfs_file_ops; /* later for read/write */
	}

	inode->i_private = node; /* 🔥 link RAM node */

	return inode;
}

/*
 * Fill superblock
 */
int vtfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *inode;
	struct vtfs_node *root;

	if (vtfs_store_init(sb))
		return -ENOMEM;

	root = vtfs_store_root(sb);
	if (!root)
		return -ENOMEM;

	inode = vtfs_make_inode(sb, root);
	if (!inode)
		return -ENOMEM;

	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}
