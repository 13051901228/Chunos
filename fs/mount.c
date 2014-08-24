#include <os/kernel.h>
#include <os/mount.h>
#include <os/file.h>
#include <os/disk.h>
#include <os/filesystem.h>
#include <os/fnode.h>
#include <sys/sys_mount.h>

struct mount_tree {
	struct mount_point *root;
	struct mutex mount_mutex;
};

static struct mount_tree mount_tree;

static int init_super_block(struct super_block *sb)
{
	init_list(&sb->list);
	init_mutex(&sb->mutex);
	init_list(&sb->fnodes);

	return 0;
}

static struct mount_point *alloc_fs_mount(char *path)
{
	struct mount_point *mount;
	int path_len = strlen(path) + 1;

	mount = kzalloc(sizeof(struct mount_point), GFP_KERNEL);
	if (!mount)
		return NULL;

	mount->path = kzalloc(path_len, GFP_KERNEL);
	if (!mount->path)
		goto release_mount;
	strncpy(mount->path, path, strlen(path));

	mount->sb = kzalloc(sizeof(struct super_block), GFP_KERNEL);
	if (!mount->sb)
		goto release_path_name;
	init_super_block(mount->sb);

	mount->fs = NULL;
	mount->flag = 0;

	return mount;

release_path_name:
	kfree(mount->path);

release_mount:
	kfree(mount);

	return NULL;
}

int do_mount(struct bdev *bdev, char *path, struct filesystem *fs, unsigned long flag)
{
	struct mount_point *mount;
	int err = 0;
	unsigned long flags;
	struct mount_point *tmp;

	/*
	 * if mount root filesystem, it is not need to check
	 * whether the given dir is exist or not, and check
	 * wether this process can open this file
	 */
	if (!strcmp(path, "/")) {
		kernel_info("mount root filesystem\n");
	} else {
		if (_sys_access(path, O_DIRECTORY))
			return -ENOENT;
	}

	mount = alloc_fs_mount(path);
	if (!mount)
		return -ENOMEM;

	mount->fs = fs;
	err = fs->fops->fill_super(bdev, fs, mount->sb);
	if (err) {
		kernel_error("Get disk superblock failed ret:%d\n", err);
		goto release_mount;
	}

	mount->sb->root_fnode = fs->fops->get_root_fnode(mount->sb);
	if (!mount->sb->root_fnode) {
		kernel_error("can not get root fnode \n");
		goto release_mount;
	}

	enter_critical(&flags);
	if (mount_tree.root) {
		tmp = mount_tree.root->next;
		mount_tree.root->next = mount;
		mount->next = tmp;
	}
	else {
		mount_tree.root = mount;
	}
	exit_critical(&flags);

	return 0;

release_mount:
	kfree(mount);

	return err;
}

static struct bdev *get_bdev_by_name(char *name)
{
	u8 disk_minor;
	u8 disk_major;

#if 0
	struct fnode *fnode;
	dev_t dev;

	fnode = get_file_fnode(name, O_RDONLY);
	if (!fnode) {
		kernel_warning("can not find file %s\n", name);
		return NULL;
	}

	dev = fnode->dev;
	release_fnode(fnode);

	return disk_get_bdev(MAJOR(dev), MINOR(dev));
#endif
	if (strncmp(name, "/dev/disk", 9))
		return NULL;
	if (name[10] != 'p')
		return NULL;

	/* TO BE DONE, no only do some basic support */
	disk_major = name[9] - '0';
	disk_minor = name[11] - '0';
	kernel_debug("disk_major %d disk_minor %d\n", disk_major, disk_minor);

	return disk_get_bdev(disk_major, disk_minor);
}

int sys_mount(char *dev_path, char *path, char *fs,
		unsigned long flag, void *data)
{
	struct bdev *bdev = NULL;
	struct filesystem *filesystem;

	if ((!dev_path) || (!path))
		return -EINVAL;

	/*
	 * Do not interpret character or block special
	 * devices on the file system.
	 */
	if (!(flag & MS_NODEV)) {
		bdev = get_bdev_by_name(dev_path);
		if (!bdev)
			return -ENODEV;
	}

	filesystem = lookup_filesystem(fs);
	if (!filesystem)
		return -ENOENT;

	return do_mount(bdev, path, filesystem, flag);
}

struct mount_point *get_mount_point(char *file_name)
{
	struct mount_point *tmp = mount_tree.root->next;
	struct mount_point *mnt = mount_tree.root;
	int file_len = strlen(file_name);
	int len;
	int len_old = 0;

	while (tmp) {
		len = strlen(tmp->path);
		printk("tmp_path %s file_name %s\n", tmp->path, file_name);
		/* if new length of mount point big than old */
		if (len > len_old) {
			if (len <= file_len) {
				if (!strncmp(tmp->path, file_name, len)) {
					mnt = tmp;
					len_old = len;
				}
			}
		}
		tmp = tmp->next;
	}

	return mnt;
}

char *get_file_fs_name(struct mount_point *mnt, char *file_name)
{
	if (strncmp(mnt->path, file_name, strlen(mnt->path)))
		return NULL;
	
	if (strlen(file_name) < (strlen(mnt->path)))
		return NULL;

	/* mount point is /home then /home/1.txt is 1.txt */
	return (file_name + (strlen(mnt->path)));
}

int mount_root(char *dev_path, char *path, char *fs, int flag)
{
	return (sys_mount(dev_path, path, fs, flag, NULL));
}

void __init_text mount_init(void)
{
	/* init system mount tree */
	memset((void *)&mount_tree, 0, sizeof(struct mount_tree));
	init_mutex(&mount_tree.mount_mutex);

	mount_root("/dev/disk0p0", "/", "lmfs", 0);
	sys_mount("devfs", "/dev", "devfs", MS_NODEV, NULL);
}
