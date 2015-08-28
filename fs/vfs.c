#include <os/kernel.h>
#include <os/file.h>
#include <os/types.h>
#include <os/disk.h>
#include <os/vfs.h>
#include <sys/dirent.h>

#ifdef DEBUG_VFS
#define vfs_debug(fmt, ...)	kernel_debug(fmt, ##__VA_ARGS__)
#else
#define vfs_debug(fmt, ...)
#endif

static block_t vfs_get_new_data_block(struct fnode *fnode)
{
	block_t block = 0;
	struct filesystem *fs = fnode_get_filesystem(fnode);

	if (fs->fops->get_block) {
		block = fs->fops->get_block(fnode);
		if (!block)
			goto out;

		fnode->blk_cnt++;
	}
out:
	return block;
}

static u32 vfs_get_data_block(struct fnode *fnode, int whence)
{
	struct filesystem *fs = fnode_get_filesystem(fnode);

	return fs->fops->get_data_block(fnode, whence);
}

static int __vfs_read_block(struct fnode *fnode, char *buffer, u32 block)
{
	struct filesystem *fs = fnode_get_filesystem(fnode);

	return fs->fops->read_block(fnode->sb, buffer, block);
}

static int __vfs_write_block(struct fnode *fnode, char *buffer, u32 block)
{
	struct filesystem *fs = fnode_get_filesystem(fnode);

	return fs->fops->write_block(fnode->sb, buffer, block);
}

static int vfs_read_block(struct fnode *fnode, char *buffer, int whence)
{
	u32 block  = 0;

	block = vfs_get_data_block(fnode, whence);
	if (!block) {
		kernel_error("can not get more data block from file\n");
		return -ENOSPC;
	}
	fnode->current_block = block;

	return __vfs_read_block(fnode, buffer, block);
}

static int vfs_write_block(struct fnode *fnode, char *buffer, int whence)
{
	u32 block  = 0;

	block = vfs_get_data_block(fnode, whence);
	if (!block) {
		kernel_error("can not get more data block from file\n");
		return -ENOSPC;
	}
	fnode->current_block = block;

	return __vfs_write_block(fnode, buffer, block);
}

static int vfs_open(struct file *file, int flag)
{
	struct fnode *fnode = file->fnode;
	
	return vfs_read_block(fnode, fnode->data_buffer, VFS_START_BLOCK);
}

static int vfs_read(struct file *file, char *buffer, size_t size)
{
	struct fnode *fnode = file->fnode;
	int err = 0, i;
	size_t real_size = size;
	s32 pos_in_buffer = fnode->rw_pos;
	size_t size_in_buffer, blocks, size_left;
	char *data_buffer = fnode->data_buffer;
	size_t buffer_size = fnode->buffer_size;

	vfs_debug("#####buffer_size:%x fnode_size:%x offset:%d#####\n",
			buffer_size, fnode->data_size,file->offset);

	if ((file->offset + size) > fnode->data_size)
		real_size = fnode->data_size - file->offset;

	if(real_size == 0) {
		kernel_info("already the end of the file\n");
		return 0;
	}

	size_in_buffer = buffer_size - pos_in_buffer;
	if (size_in_buffer > real_size) {
		size_in_buffer = real_size;
		blocks = 0;
		size_left = 0;
	} else {
		blocks = (real_size - size_in_buffer) / buffer_size;
		size_left = (real_size - size_in_buffer) % buffer_size;
	}

	vfs_debug("real_size:%d size_in_buffer:%d blocks:%d size_left:%d\n",
		  real_size, size_in_buffer, blocks, size_left);

	memcpy(buffer, data_buffer + pos_in_buffer, size_in_buffer);
	buffer += size_in_buffer;

	for (i = 0; i < blocks; i++) {
		err = vfs_read_block(fnode, buffer, VFS_NEXT_BLOCK);
		if (err)
			return err;

		buffer += buffer_size;
	}

	if (size_left) {
		err = vfs_read_block(fnode, data_buffer, VFS_NEXT_BLOCK);
		if (err)
			return err;

		memcpy(buffer, data_buffer, size_left);
		size_in_buffer = buffer_size - size_left;
	}

	if (size_in_buffer == buffer_size) {
		/* sinc all buffer is readed read a new block to the fnode*/
		err = vfs_read_block(fnode, data_buffer, VFS_NEXT_BLOCK);
		if (err)
			return err;
	}

	file->offset += real_size;
	fnode->rw_pos = (pos_in_buffer + real_size) % buffer_size;

	vfs_debug("##### fnode_pos:%x file_pos:%x#####\n",
			fnode->rw_pos, file->offset);

	return real_size;
}

static int vfs_write(struct file *file, char *buffer, size_t size)
{
	struct fnode *fnode = file->fnode;
	size_t ret = 0;
	s32 pos = fnode->rw_pos;
	size_t size_in_buffer, blocks, size_left;
	u32 next_block = 0;
	int i, allocated_new = 0;
	char *data_buffer = fnode->data_buffer;
	size_t buffer_size = fnode->buffer_size;

	size_in_buffer = buffer_size - pos;
	if (size_in_buffer > size) {
		size_in_buffer = size;
		blocks = 0;
		size_left = 0;
	} else {
		blocks = (size - size_in_buffer) / buffer_size;
		size_left = (size - size_in_buffer) % buffer_size;
	}

	memcpy(data_buffer + pos, buffer, size_in_buffer);
	ret = vfs_write_block(fnode, data_buffer, VFS_CURRENT_BLOCK);
	if (ret)
		return -EIO;

	buffer += size_in_buffer;

	for (i = 0; i < blocks; i ++) {
		if (!allocated_new) {
			next_block = vfs_get_data_block(fnode, VFS_NEXT_BLOCK);
			if (next_block == 0) {
				allocated_new = 1;
				next_block = vfs_get_new_data_block(fnode);
				if (!next_block)
					return -ENOSPC;
			}
		} else {
			next_block = vfs_get_new_data_block(fnode);
			if (!next_block)
				return -EFBIG;
		}

		//memcpy(data_buffer, buffer, buffer_size);
		if (__vfs_write_block(fnode, buffer, next_block))
			return -EIO;

		buffer += buffer_size;
	}

	if (size_left) {
		if (!allocated_new) {
			next_block = vfs_get_data_block(fnode, VFS_NEXT_BLOCK);
			if (next_block == 0) {
				allocated_new = 1;
				next_block = vfs_get_new_data_block(fnode);
				if (!next_block)
					return -ENOSPC;
			}
		} else {
			next_block = vfs_get_new_data_block(fnode);
			if (!next_block)
				return -EFBIG;
		}

		memcpy(data_buffer, buffer, size_left);
		if (__vfs_write_block(fnode, data_buffer, next_block))
			return -EIO;
	}
	
	/* TO BE DOWN */
	if (!allocated_new) {
		/* free unused buffer id needed */
	}

	/* update fnode information if needed */
	fnode->blk_cnt = baligin(file->offset + size, buffer_size) / buffer_size;
	fnode->data_size = file->offset + size;
	file->offset += size;
	fnode->rw_pos = (pos + size) % buffer_size;

	update_fnode(fnode);
	update_super_block(fnode->sb);

	return ret;
}

static int vfs_seek(struct file *file, offset_t offset, int whence)
{
	struct fnode *fnode = file->fnode;
	struct filesystem *fs = fnode_get_filesystem(fnode);
	offset_t new = offset;
	int err = 0;
	int direction;
	offset_t new_off;
	u32 left = 0, blocks = 0, not_aligin = 0;
	s32 pos = fnode->rw_pos;
	int i;
	u32 next_block = 0;
	char *data_buffer = fnode->data_buffer;
	size_t buffer_size = fnode->buffer_size;

	/* 1 is front 0 is back will define malloc for them later */
	switch (whence) {
	case SEEK_SET:
		if (offset > fnode->data_size)
			return -EINVAL;

		if (offset < file->offset) {
			direction = 1;
			new_off = file->offset - offset;
		} else {
			new_off = offset - file->offset;
			direction = 0;
		}
		new = offset;
		break;
	case SEEK_CUR:
		if ((file->offset + offset) > fnode->data_size)
			new_off = fnode->data_size - file->offset;
		else
			new_off = offset;
		direction = 0;
		new = file->offset + offset;
		break;
	case SEEK_END:
		new_off = fnode->data_size - file->offset;
		direction = 0;
		new = fnode->data_size;
		break;
	default:
		break;
	}

	/* if nothing need to do */
	if (new_off <= 0)
		return 0;

	/* if fs do not support prev next block operation */
	if (direction) {
		if (!(fs->flag & FS_FOPS_PREV_BLOCK)) {
			if (new_off > pos) {
				vfs_debug("change the direction\n");
				new_off = file->offset - new_off;
				fnode->rw_pos = pos = 0;
				direction = 0; 
				vfs_read_block(fnode, data_buffer, VFS_START_BLOCK);
			}
		}
	}

	vfs_debug("direction %d new_off %d offset:%d pos:%d\n",
			direction, new_off, file->offset, pos);

	if (direction) {
		left = pos;
		if (new_off > left) {
			not_aligin = (new_off - left) % buffer_size;
			blocks = baligin(new_off - left, buffer_size) / buffer_size;

			vfs_debug("front not_aligin:%x blocks:%x\n", not_aligin, blocks);

			for (i = 0; i < blocks; i++) {
				next_block = vfs_get_data_block(fnode, VFS_PREV_BLOCK);
				if (!next_block)
					return -ENOSPC;
			}

			err = __vfs_read_block(fnode, data_buffer, next_block);
			if (err)
				return -EIO;

			fnode->rw_pos = buffer_size - not_aligin;
		} else {
			fnode->rw_pos -= new_off;
		}

	} else {
		left = buffer_size - pos;
		if (new_off >= left) {
			not_aligin = (new_off - left)  % buffer_size;
			blocks = baligin(new_off - left, buffer_size) / buffer_size;

			/* need to consider the boundary case when new_off == N * buffer_size */
			if (not_aligin == 0)
				blocks++;

			vfs_debug("back not_aligin:%x blocks:%x\n", not_aligin, blocks);
			for (i = 0; i < blocks; i++) {
				next_block = vfs_get_data_block(fnode, VFS_NEXT_BLOCK);
				if (!next_block)
					return -ENOSPC;
			}

			err = __vfs_read_block(fnode, data_buffer, next_block);
			if (err)
				return -EIO;

			fnode->rw_pos = not_aligin;
		} else {
			fnode->rw_pos += new_off;
		}
	}
	fnode->rw_pos %= buffer_size;

	file->offset = new;
	vfs_debug("file_offset:%x fnode_offset:%x\n",
			file->offset, fnode->rw_pos);
	return file->offset;
}

static size_t vfs_mmap(struct file *file, char *buffer,
		size_t size, offset_t offset)
{
	int err;

	if ((file->offset != offset)) {
		err = vfs_seek(file, offset, SEEK_SET);
		if (err)
			return err;
	}

	return vfs_read(file, buffer, size);
}

static size_t vfs_msync(struct file *file, char *buffer,
			size_t size, offset_t offset)
{
	int err;

	/* TBC */
	if (file->offset != offset) {
		err = vfs_seek(file, offset, SEEK_SET);
		if (err)
			return err;
	}

	return vfs_write(file, buffer, size);
}

static int vfs_ioctl(struct file *file, u32 cmd, void *arg)
{
	/* TO BE IMPLEMENT */
	return 0;
}

static int vfs_close(struct file *file)
{
	struct filesystem *fs = fnode_get_filesystem(file->fnode);

	if (fs->fops->release_fnode)
		fs->fops->release_fnode(file->fnode);

	/* TO BE IMPLEMENT */
	release_fnode(file->fnode);
	kfree(file);

	return 0;
}

int vfs_getdents(struct file *file, char *buffer, size_t count)
{
	int dent_count = 0, sum = 0;
	struct fnode *fnode = file->fnode;
	char *data_buffer = fnode->data_buffer;
	struct filesystem *fs = fnode_get_filesystem(fnode);
	size_t handle_size = 0;
	int err = 0;
	struct os_dirent *dent;

	if (fnode->type != DT_DIR)
		return -ENOTDIR;

	if (!count)
		return -EINVAL;

	/* if there is no more size to read */
	if ((fnode->rw_pos == fnode->buffer_size) &&
			(fnode->current_block == 0))
		return -ENOSPC;

	while ((dent_count + sizeof(struct os_dirent) + 256) < count) {

		dent = (struct os_dirent *)buffer;
		memset(&dent->d_name[1], 0, 256);
		handle_size = fs->fops->getdents(fnode,
				(struct dirent *)(dent->d_name),
				data_buffer);
		/* no more data or error */
		if (!handle_size)
			break;

		file->offset += handle_size;
		fnode->rw_pos += handle_size;
		data_buffer += handle_size;

		err = strlen(&dent->d_name[1]);
		if (err) {
			/* what is d_ino means, who knows */
			dent->d_ino = (long)fnode;
			dent->d_off = file->offset;
			dent->d_reclen = err + sizeof(struct os_dirent);

			dent->d_reclen = baligin(dent->d_reclen, 4);

			sum += dent->d_reclen;
			buffer += dent->d_reclen;
			dent_count += dent->d_reclen;
		}

		if (fnode->rw_pos >= fnode->buffer_size) {
			/* read the next block data */
			err = vfs_read_block(fnode, fnode->data_buffer, VFS_NEXT_BLOCK);
			if (err) {
				fnode->current_block = 0;
				goto out;
			} else {
				fnode->rw_pos = 0;
				data_buffer = fnode->data_buffer;
			}
		}
		
	}

out:
	return sum;
}

int vfs_stat(struct file *file, struct stat *stat)
{
	return 0;
}

struct file_operations vfs_ops = {
	.open	= vfs_open,
	.read	= vfs_read,
	.write	= vfs_write,
	.ioctl	= vfs_ioctl,
	.mmap	= vfs_mmap,
	.msync	= vfs_msync,
	.seek	= vfs_seek,
	.close	= vfs_close,
	.stat	= vfs_stat,
};
