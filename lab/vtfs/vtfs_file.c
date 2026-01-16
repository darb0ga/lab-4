#include "vtfs.h"
#include <linux/uaccess.h>
#include <linux/slab.h>

static ssize_t vtfs_read(struct file *file, char __user *buf,
                         size_t len, loff_t *ppos)
{
	struct inode *inode = file_inode(file);
	struct vtfs_fileobj *f = inode->i_private;
	size_t available;

	if (!f || !f->data)
		return 0;

	if (*ppos >= f->size)
		return 0;

	available = f->size - *ppos;
	if (len > available)
		len = available;

	if (copy_to_user(buf, f->data + *ppos, len))
		return -EFAULT;

	*ppos += len;
	return len;
}

static ssize_t vtfs_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *ppos)
{
	struct inode *inode = file_inode(file);
	struct vtfs_fileobj *f = inode->i_private;
	size_t new_size;
	char *new_data;

	if (!f)
		return -EINVAL;

	if (file->f_flags & O_APPEND)
		*ppos = f->size;

	if (*ppos == 0) {
		kfree(f->data);
		f->data = NULL;
		f->size = 0;
	}

	new_size = *ppos + len;

	new_data = krealloc(f->data, new_size, GFP_KERNEL);
	if (!new_data)
		return -ENOMEM;

	f->data = new_data;

	if (copy_from_user(f->data + *ppos, buf, len))
		return -EFAULT;

	*ppos += len;
	f->size = max(f->size, new_size);

	inode_set_ctime_current(inode);
	inode->i_size = f->size;

	return len;
}

const struct file_operations vtfs_file_fops = {
	.owner = THIS_MODULE,
	.read  = vtfs_read,
	.write = vtfs_write,
	.llseek = default_llseek,
};