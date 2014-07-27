/*
 * driver/tty.c
 *
 * Create by Le Min at 07/26/2014
 */

#include <os/kernel.h>
#include <os/device.h>
#include <os/spin_lock.h>
#include <os/tty.h>

#define TTY_NR	8
static struct tty *tty_table[TTY_NR];

static int tty_open(struct file *file, int flag)
{
	int minor = MINOR(file->fnode->dev);
	struct tty *tty;

	tty = tty_table[minor];
	file->private_data = tty;

	return tty->tty_ops->open(tty);
}

static size_t tty_read(struct file *file, char *buf, size_t size)
{
	return size;
}

static size_t tty_write(struct file *file, char *buf, size_t size)
{
	struct tty *tty = (struct tty *)file->private_data;
	int i = 0;
	unsigned long flags;

	spin_lock_irqsave(&tty->tty_lock, &flags);

	while (*buf)
		tty->tty_ops->put_char(tty, buf[i++]);

	spin_unlock_irqstore(&tty->tty_lock, &flags);

	return size;
}

static int tty_ioctl(struct file *file, int cmd, void *arg)
{
	return 0;
}

struct file_operations tty_fops = {
	.open	= tty_open,
	.read	= tty_read,
	.write	= tty_write,
	.ioctl	= tty_ioctl,
};

struct tty *allocate_tty(void)
{
	int i;
	int minor = -1;
	struct tty *tty;
	struct device *dev;

	for (i = 0; i < TTY_NR; i++) {
		if (tty_table[i] == NULL) {
			minor = i;
			break;
		}
	}

	if (i < 0)
		return NULL;
	
	tty_table[i] = (struct tty *)0x12345678;

	dev = alloc_device(TTY_MAJOR, minor);
	if (!dev)
		return NULL;

	tty = kzalloc(sizeof(struct tty), GFP_KERNEL);
	if (!tty) {
		goto err_alloc_tty;
	}

	tty->dev = dev;
	tty->tty_ops = NULL;
	tty->is_console = 1;
	spin_lock_init(&tty->tty_lock);

	return tty;

err_alloc_tty:
	kfree(dev);

	return NULL;
}

int register_tty(struct tty *tty)
{
	int err;
	
	if (!tty->tty_ops) {
		kernel_error("No tty ops\n");
		return -EINVAL;
	}

	err = register_device(tty->dev);
	if (err)
		return err;
	
	if (!strcmp(tty->dev->name, "ttyS0"))
		tty->is_console = 1;

	tty_table[MINOR(tty->dev->dev)] = tty;

	return 0;
}

static int tty_core_init(void)
{
	struct device_class *class;
	int err = 0;

	class = allocate_device_class(TTY_MAJOR);
	if (!class)
		return -ENOMEM;

	memcpy(class->name, "ttyS", 4);
	class->fops = &tty_fops;
	
	err = register_chardev_class(class);
	if (err) {
		kfree(class);
		return err;
	}

	return 0;
}

int tty_flush_log(char *buf, int size)
{
	int i;
	struct tty *tty;
	char *tmp;
	int printed = 0;

	for (i = 0; i < TTY_NR; i++) {
		if (!(tty = tty_table[i]))
			continue;

		if (tty->is_console) {
			printed = 1;
			tmp = buf;
			while (*tmp)
				tty->tty_ops->put_char(tty, *tmp++);
		}
	}

	return (printed ? size : 0);
}

fs_initcall(tty_core_init);
