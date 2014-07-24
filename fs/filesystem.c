#include <os/kernel.h>
#include <os/mutex.h>
#include <os/filesystem.h>
#include <os/errno.h>

LIST_HEAD(filesystem_head);

int update_super_block(struct super_block *sb)
{
	return 0;
}

struct filesystem *lookup_filesystem(char *name)
{
	struct filesystem *fs = NULL;
	struct list_head *list;

	list_for_each(&filesystem_head, list) {
		fs = list_entry(list, struct filesystem, list);
		if (!strcmp(name, fs->name))
			break;
	}

	return fs;
}

int register_filesystem(struct filesystem *filesystem)
{
	if (!filesystem || !filesystem->fops->fill_super || !filesystem->fops)
		return -EINVAL;

	list_add_tail(&filesystem_head, &filesystem->list);
	kernel_info("register filesystem %s\n", filesystem->name);

	return 0;
}
