#include "vtfs.h"
#include <linux/printk.h>

static int __init vtfs_init(void)
{
	int ret;

	pr_info("[vtfs] init\n");
	ret = register_filesystem(&vtfs_fs_type);
	pr_info("[vtfs] register_filesystem ret=%d\n", ret);
	return ret;
}

static void __exit vtfs_exit(void)
{
	int ret;

	ret = unregister_filesystem(&vtfs_fs_type);
	pr_info("[vtfs] exit unregister_filesystem ret=%d\n", ret);
}

module_init(vtfs_init);
module_exit(vtfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dasha");
MODULE_DESCRIPTION("VTFS: simple in-RAM filesystem (fedfs-inspired)");