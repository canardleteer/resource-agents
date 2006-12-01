/******************************************************************************
*******************************************************************************
**
**  Copyright (C) Sistina Software, Inc.  1997-2003  All rights reserved.
**  Copyright (C) 2004 Red Hat, Inc.  All rights reserved.
**
**  This copyrighted material is made available to anyone wishing to use,
**  modify, copy, or redistribute it subject to the terms and conditions
**  of the GNU General Public License v.2.
**
*******************************************************************************
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <stdint.h>
#include <inttypes.h>

#include <linux/types.h>
#include "gfs2_quota.h"

#define __user

#include "copyright.cf"


/*  Constants  */

#define OPTION_STRING ("bdf:g:hkl:mnsu:V")

char *prog_name;

/**
 * This function is for libgfs2's sake.
 */
void print_it(const char *label, const char *fmt, const char *fmt2, ...)
{
        va_list args;

        va_start(args, fmt2);
        printf("%s: ", label);
        vprintf(fmt, args);
        va_end(args);
}

/**
 * print_usage - print usage info to the user
 *
 */

static void
print_usage()
{
	printf("Usage:\n");
	printf("\n");
	printf("%s <list|sync|get|limit|warn|check|init> [options]\n",
	       prog_name);
	printf("\n");
	printf("Actions:\n");
	printf("  list             list the whole quota file\n");
	printf("  sync             sync out unsynced quotas\n");
	printf("  get              get quota values for an ID\n");
	printf("  limit            set a quota limit value for an ID\n");
	printf("  warn             set a quota warning value for an ID\n");
	printf("  check            check the quota file\n");
	printf("  init             initialize the quota file\n");
	printf("\n");
	printf("Options:\n");
	printf("  -b               sizes are in FS blocks\n");
	printf("  -f <directory>   the filesystem to work on\n");
	printf("  -g <gid>         get/set a group ID\n");
	printf("  -h               Print this help, then exit\n");
	printf("  -k               sizes are in KB\n");
	printf("  -l <size>        the new limit or warn value\n");
	printf("  -m               sizes are in MB\n");
	printf("  -n               print out UID/GID numbers instead of names\n");
	printf("  -s               sizes are in 512-byte blocks\n");
	printf("  -u <uid>         get/set a user ID\n");
	printf("  -V               Print program version information, then exit\n");
}

/**
 * check_for_gfs2 - Check to see if a descriptor is a file on a GFS2 filesystem
 * @fd: the file descriptor
 * @path: the path used to open the descriptor
 *
 */

/**
 * decode_arguments - parse command line arguments
 * @argc: well, it's argc...
 * @argv: well, it's argv...
 * @comline: the structure filled in with the parsed arguments
 *
 * Function description
 *
 * Returns: what is returned
 */

static void
decode_arguments(int argc, char *argv[], commandline_t *comline)
{
	int cont = TRUE;
	int optchar;

	while (cont) {
		optchar = getopt(argc, argv, OPTION_STRING);

		switch (optchar) {
		case 'u':
			comline->id_type = GQ_ID_USER;
			comline->id = name_to_id(TRUE, optarg, comline->numbers);
			break;

		case 'g':
			comline->id_type = GQ_ID_GROUP;
			comline->id = name_to_id(FALSE, optarg, comline->numbers);
			break;

		case 'l':
			if (!isdigit(*optarg))
				die("argument to -l must be a number\n");
			sscanf(optarg, "%"SCNu64, &comline->new_value);
			comline->new_value_set = TRUE;
			break;

		case 'f':
			if (!realpath(optarg, comline->filesystem))
				die("can't find %s: %s\n", optarg,
				    strerror(errno));
			break;

		case 'm':
			comline->units = GQ_UNITS_MEGABYTE;
			break;

		case 'k':
			comline->units = GQ_UNITS_KILOBYTE;
			break;

		case 'b':
			comline->units = GQ_UNITS_FSBLOCK;
			break;

		case 's':
			comline->units = GQ_UNITS_BASICBLOCK;
			break;

		case 'n':
			comline->numbers = TRUE;
			break;

		case 'V':
			printf("gfs2_quota %s (built %s %s)\n", GFS2_RELEASE_NAME,
			       __DATE__, __TIME__);
			printf("%s\n", REDHAT_COPYRIGHT);
			exit(EXIT_SUCCESS);
			break;

		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
			break;

		case ':':
		case '?':
			fprintf(stderr, "Please use '-h' for usage.\n");
			exit(EXIT_FAILURE);
			break;

		case EOF:
			cont = FALSE;
			break;

		default:
			die("unknown option: %c\n", optchar);
			break;
		};
	}

	while (optind < argc) {
		if (strcmp(argv[optind], "list") == 0 ||
		    strcmp(argv[optind], "dump") == 0) {
			if (comline->operation)
				die("can't specify two operations\n");
			comline->operation = GQ_OP_LIST;
		} else if (strcmp(argv[optind], "sync") == 0) {
			if (comline->operation)
				die("can't specify two operations\n");
			comline->operation = GQ_OP_SYNC;
		} else if (strcmp(argv[optind], "get") == 0) {
			if (comline->operation)
				die("can't specify two operations\n");
			comline->operation = GQ_OP_GET;
		} else if (strcmp(argv[optind], "limit") == 0) {
			if (comline->operation)
				die("can't specify two operations\n");
			comline->operation = GQ_OP_LIMIT;
		} else if (strcmp(argv[optind], "warn") == 0) {
			if (comline->operation)
				die("can't specify two operations\n");
			comline->operation = GQ_OP_WARN;
		} else if (strcmp(argv[optind], "check") == 0) {
			if (comline->operation)
				die("can't specify two operations\n");
			comline->operation = GQ_OP_CHECK;
		} else if (strcmp(argv[optind], "init") == 0) {
			if (comline->operation)
				die("can't specify two operations\n");
			comline->operation = GQ_OP_INIT;
		} else
			die("unknown option %s\n", argv[optind]);

		optind++;
	}
}

/**
 * print_quota - Print out a quota entry
 * @comline: the struct containing the parsed command line arguments
 * @user: TRUE if this is a user quota, FALSE if it's a group quota
 * @id: the ID
 * @q: the quota value
 * @sb: the superblock of the filesystem this quota belongs to
 *
 */

static void
print_quota(commandline_t *comline,
	    int user, uint32_t id,
	    struct gfs2_quota *q,
	    struct gfs2_sb *sb)
{
	printf("%-5s %10s:  ", (user) ? "user" : "group",
	       id_to_name(user, id, comline->numbers));

	switch (comline->units) {
	case GQ_UNITS_MEGABYTE:
		printf("limit: %-10.1f warn: %-10.1f value: %-10.1f\n",
		       (double) q->qu_limit * sb->sb_bsize / 1048576,
		       (double) q->qu_warn * sb->sb_bsize / 1048576,
		       (double) q->qu_value * sb->sb_bsize / 1048576);
		break;

	case GQ_UNITS_KILOBYTE:
		if (sb->sb_bsize == 512)
			printf("limit: %-10"PRIu64" warn: %-10"PRIu64"value: %-10"PRId64"\n",
			       q->qu_limit / 2,
			       q->qu_warn / 2,
			       q->qu_value / 2);
		else
			printf("limit: %-10"PRIu64" warn: %-10"PRIu64"value: %-10"PRId64"\n",
			       q->qu_limit << (sb->sb_bsize_shift - 10),
			       q->qu_warn << (sb->sb_bsize_shift - 10),
			       q->qu_value << (sb->sb_bsize_shift - 10));
		break;

	case GQ_UNITS_FSBLOCK:
		printf("limit: %-10"PRIu64" warn: %-10"PRIu64" value: %-10"PRId64"\n",
		       q->qu_limit, q->qu_warn, q->qu_value);
		break;

	case GQ_UNITS_BASICBLOCK:
		printf("limit: %-10"PRIu64" warn: %-10"PRIu64" value: %-10"PRId64"\n",
		       q->qu_limit << (sb->sb_bsize_shift - 9),
		       q->qu_warn << (sb->sb_bsize_shift - 9),
		       q->qu_value << (sb->sb_bsize_shift - 9));
		break;

	default:
		die("bad units\n");
		break;
	}
}

void
lock_for_admin()
{
        int error;

        for (;;) {

                metafs_fd = open(metafs_path, O_RDONLY | O_NOFOLLOW);
                if (metafs_fd < 0)
                        die("can't open %s: %s\n",
                            metafs_path, strerror(errno));

                error = flock(metafs_fd, LOCK_EX);
                if (error)
                        die("can't flock %s: %s\n", metafs_path,
                            strerror(errno));

                break;
        }
}

int 
find_gfs2_meta(const char *mnt)
{
	FILE *fp = fopen("/proc/mounts", "r");
	char name[] = "gfs2meta";
	char buffer[BUF_SIZE];
	char fstype[80], mfsoptions[BUF_SIZE];
	char meta_device[BUF_SIZE];
	int fsdump, fspass;

	metafs_mounted = 0;

	if (fp == NULL) {
		perror("open: /proc/mounts");
		exit(EXIT_FAILURE);
	}
	while ((fgets(buffer, 4095, fp)) != NULL) {
		buffer[4095] = 0;
		if (strstr(buffer, name) == 0)
			continue;

		if (sscanf(buffer, "%s %s %s %s %d %d", meta_device, 
			   metafs_path, fstype,mfsoptions, &fsdump, 
			   &fspass) != 6)
			continue;
		
		if (strcmp(meta_device, device_name) != 0
		    && strcmp(meta_device, mnt) != 0)
			continue;
		
		metafs_mounted = 1;
		
		fclose(fp);
		return TRUE;
	}
	fclose(fp);
	return FALSE;
}

static int
dir_exists(const char *dir)
{
	int fd, ret;
	struct stat statbuf;
	fd = open(dir, O_RDONLY);
	if (fd<0) { 
		if (errno == ENOENT)
			return 0;
		die("Couldn't open %s : %s\n", dir, strerror(errno));
	}
	ret = fstat(fd, &statbuf);
	if (ret)
		die("stat failed on %s : %s\n", dir, strerror(errno));
	if (S_ISDIR(statbuf.st_mode)) {
		close(fd);
		return 1;
	}
	close(fd);
	die("%s exists, but is not a directory. Cannot mount metafs here\n", dir);
}

void 
mount_gfs2_meta()	 
{
	int ret;
	/* mount the meta fs */
	if (!dir_exists(meta_mount)) {
		ret = mkdir(meta_mount, 0700);
		if (ret)
			die("Couldn't create %s : %s\n", meta_mount,
			    strerror(errno));
	}
		
	ret = mount(device_name, meta_mount, "gfs2meta", 0, NULL);
	if (ret)
		die("Couldn't mount %s : %s\n", meta_mount,
		    strerror(errno));
	strcpy(metafs_path, meta_mount);
}

void
cleanup()
{
	int ret;
	if (!metafs_mounted) { /* was mounted by us */
		ret = umount(meta_mount);
		if (ret)
			fprintf(stderr, "Couldn't unmount %s : %s\n", meta_mount, 
			    strerror(errno));
	}
}

void
check_for_gfs2(const char *mnt)
{
	FILE *fp = fopen("/proc/mounts", "r");
	char buffer[4096];
	char fstype[80];
	int fsdump, fspass, ret;

	if (fp == NULL) {
		perror("open: /proc/mounts");
		exit(EXIT_FAILURE);
	}
	while ((fgets(buffer, 4095, fp)) != NULL) {
		buffer[4095] = 0;

		if (strstr(buffer, "0") == 0)
			continue;

		if ((ret = sscanf(buffer, "%s %s %s %s %d %d", device_name, fspath, 
				  fstype, fsoptions, &fsdump, &fspass)) != 6) 
			continue;

		if (strcmp(fstype, "gfs2") != 0)
			continue;

		if (strcmp(fspath, mnt) != 0)
			continue;

		fclose(fp);
		if (strncmp(device_name, "/dev/loop", 9) == 0)
			die("Cannot add journal(s) to a loopback GFS mount\n");
		
		return;
	}
	fclose(fp);
	die("gfs2 Filesystem %s not found\n", mnt);
}

void 
read_superblock(struct gfs2_sb *sb)
{
	int fd;
	char buf[PATH_MAX];
	
	fd = open(device_name, O_RDONLY);
	if (fd < 0) {
		die("Could not open the block device %s: %s\n",
			device_name, strerror(errno));
	}
	do_lseek(fd, 0x10 * 4096);
	do_read(fd, buf, PATH_MAX);
	gfs2_sb_in(sb, buf);

	close(fd);
}

/**
 * do_list - List all the quota data for a filesystem
 * @comline: the struct containing the parsed command line arguments
 *
 */

static void 
do_list(commandline_t *comline)
{
	int fd;
	struct gfs2_sb sb;
	struct gfs2_quota q;
	char buf[sizeof(struct gfs2_quota)];
	uint64_t offset;
	uint32_t id;
	int pass = 0;
	int error = 0;
	char quota_file[BUF_SIZE];
	
	if (!*comline->filesystem)
		die("need a filesystem to work on\n");

	check_for_gfs2(comline->filesystem);
	read_superblock(&sb);
	if (!find_gfs2_meta(comline->filesystem))
		mount_gfs2_meta();
	lock_for_admin();
	
	strcpy(quota_file, metafs_path);
	strcat(quota_file, "/quota");

	fd = open(quota_file, O_RDONLY);
	if (fd < 0) {
		close(metafs_fd);
		cleanup();
		die("can't open file %s: %s\n", quota_file,
		    strerror(errno));
	}
	for (pass=0; pass<2; pass++) {
		if (!pass)
			offset = 0;
		else
			offset = sizeof(struct gfs2_quota);

		do {
			memset(buf, 0, sizeof(struct gfs2_quota));

			/* read hidden quota file here */
			lseek(fd, offset, SEEK_SET);
			error = read(fd, buf, sizeof(struct gfs2_quota));

			gfs2_quota_in(&q, buf);

			id = (offset / sizeof(struct gfs2_quota)) >> 1;

			if (q.qu_limit || q.qu_warn || q.qu_value)
				print_quota(comline, (pass) ? FALSE : TRUE, id,
					    &q, &sb);

			offset += 2 * sizeof(struct gfs2_quota);
		} while (error == sizeof(struct gfs2_quota));
	}

	close(fd);
	close(metafs_fd);
	cleanup();
}

/**
 * do_get_one - Get a quota value from one FS
 * @comline: the struct containing the parsed command line arguments
 * @filesystem: the filesystem to get from
 *
 */

static void 
do_get_one(commandline_t *comline, char *filesystem)
{
	int fd;
	char buf[256];
	struct gfs2_quota q;
	struct gfs2_sb sb;
	uint64_t offset;
	int error;
	char quota_file[BUF_SIZE];

	check_for_gfs2(filesystem);
	read_superblock(&sb);
	if (!find_gfs2_meta(filesystem))
		mount_gfs2_meta();
	lock_for_admin();
	
	strcpy(quota_file, metafs_path);
	strcat(quota_file, "/quota");

	fd = open(quota_file, O_RDONLY);
	if (fd < 0) {
		close(metafs_fd);
		cleanup();
		die("can't open file %s: %s\n", quota_file,
		    strerror(errno));
	}

	if (comline->id_type == GQ_ID_USER)
		offset = (2 * (uint64_t)comline->id) * sizeof(struct gfs2_quota);
	else
		offset = (2 * (uint64_t)comline->id + 1) * sizeof(struct gfs2_quota);

	memset(&q, 0, sizeof(struct gfs2_quota));
	
	lseek(fd, offset, SEEK_SET);
	error = read(fd, buf, sizeof(struct gfs2_quota));
	if (error < 0) {
		close(fd);
		close(metafs_fd);
		cleanup();
		die("can't get quota info (%d): %s\n",
		    error, strerror(errno));
	}

	gfs2_quota_in(&q, buf);


	print_quota(comline,
		    (comline->id_type == GQ_ID_USER), comline->id,
		    &q, &sb);

	close(fd);
	close(metafs_fd);
	cleanup();	
}

/**
 * do_get - Get a quota value
 * @comline: the struct containing the parsed command line arguments
 *
 */

static void
do_get(commandline_t *comline)
{
	int first = TRUE;

	if (*comline->filesystem)
		do_get_one(comline, comline->filesystem);
	else {
		char buf[256], device[256], path[256], type[256];
		FILE *file;

		file = fopen("/proc/mounts", "r");
		if (!file)
			die("can't open /proc/mounts: %s\n", strerror(errno));

		while (fgets(buf, 256, file)) {
			if (sscanf(buf, "%s %s %s", device, path, type) != 3)
				continue;
			if (strcmp(type, "gfs2") != 0)
				continue;

			if (first)
				first = FALSE;
			else
				printf("\n");

			printf("%s\n", path);
			do_get_one(comline, path);
		}

		fclose(file);
	}
}

/**
 * do_sync_one - sync the quotas on one GFS2 filesystem
 * @path: a file/directory in the filesystem
 *
 */
static void 
do_sync_one(char *filesystem)
{
	int fd;
	struct gfs2_sb sb;
	char sys_quota_sync[PATH_MAX];

	check_for_gfs2(filesystem);
	read_superblock(&sb);
	sprintf(sys_quota_sync, "%s%s%s", 
		"/sys/fs/gfs2/", sb.sb_locktable, "/quota_sync");
	
	fd = open(sys_quota_sync, O_WRONLY);
	if (fd < 0)
		die("can't open file %s: %s\n", sys_quota_sync, strerror(errno));
	
	if (write(fd,(void*)"1", 1) != 1)
		die("failed to write to %s: %s\n", 
		    sys_quota_sync, strerror(errno));
	
	close(fd);
}

/**
 * do_sync - sync out unsyned quotas
 * @comline: the struct containing the parsed command line arguments
 *
 */

void
do_sync(commandline_t *comline)
{
	sync();

	if (*comline->filesystem)
		do_sync_one(comline->filesystem);
	else {
		char buf[256], device[256], path[256], type[256];
		FILE *file;

		file = fopen("/proc/mounts", "r");
		if (!file)
			die("can't open /proc/mounts: %s\n", strerror(errno));

		while (fgets(buf, 256, file)) {
			if (sscanf(buf, "%s %s %s", device, path, type) != 3)
				continue;
			if (strcmp(type, "gfs2") != 0)
				continue;

			do_sync_one(path);
		}

		fclose(file);
	}
}

/**
 * do_set - Set a quota value
 * @comline: the struct containing the parsed command line arguments
 *
 */

static void
do_set(commandline_t *comline)
{
	int fd, fd1;
	uint64_t offset;
	struct gfs2_sb sb;
	uint64_t new_value;
	int error;
	char quota_file[BUF_SIZE];
	char sys_q_refresh[BUF_SIZE];
	
	if (!*comline->filesystem)
		die("need a filesystem to work on\n");
	if (!comline->new_value_set)
		die("need a new value\n");

	check_for_gfs2(comline->filesystem);
	read_superblock(&sb);
	if (!find_gfs2_meta(comline->filesystem))
		mount_gfs2_meta();
	lock_for_admin();
	
	strcpy(quota_file, metafs_path);
	strcat(quota_file, "/quota");

	fd = open(quota_file, O_WRONLY);
	if (fd < 0) {
		close(metafs_fd);
		cleanup();
		die("can't open file %s: %s\n", quota_file,
		    strerror(errno));
	}
	
	switch (comline->id_type) {
	case GQ_ID_USER:
		offset = (2 * (uint64_t)comline->id) * sizeof(struct gfs2_quota);
		break;

	case GQ_ID_GROUP:
		offset = (2 * (uint64_t)comline->id + 1) * sizeof(struct gfs2_quota);
		break;

	default:
		fprintf(stderr, "invalid user/group ID\n");
		goto out;
	}

	switch (comline->operation) {
	case GQ_OP_LIMIT:
		offset += (unsigned long)(&((struct gfs2_quota *) NULL)->qu_limit);
		break;

	case GQ_OP_WARN:
		offset += (unsigned long)(&((struct gfs2_quota *) NULL)->qu_warn);
		break;

	default:
		fprintf(stderr, "invalid operation\n");
		goto out;
	};

	switch (comline->units) {
	case GQ_UNITS_MEGABYTE:
		new_value = comline->new_value << (20 - sb.sb_bsize_shift);
		break;

	case GQ_UNITS_KILOBYTE:
		if (sb.sb_bsize == 512)
			new_value = comline->new_value * 2;
		else
			new_value = comline->new_value >> (sb.sb_bsize_shift - 10);
		break;

	case GQ_UNITS_FSBLOCK:
		new_value = comline->new_value;
		break;

	case GQ_UNITS_BASICBLOCK:
		new_value = comline->new_value >> (sb.sb_bsize_shift - 9);
		break;

	default:
		fprintf(stderr, "bad units\n");
		goto out;
	}

	new_value = cpu_to_be64(new_value);

	lseek(fd, offset, SEEK_SET);
	error = write(fd, (char*)&new_value, sizeof(uint64_t));
	if (error != sizeof(uint64_t)) {
		fprintf(stderr, "can't write quota file (%d): %s\n",
		    error, strerror(errno));
		goto out;
	}

	/* Write "1" to sysfs quota refresh file to refresh gfs quotas */
	sprintf(sys_q_refresh, "%s%s%s", "/sys/fs/gfs2/", sb.sb_locktable, 
		comline->id_type == GQ_ID_USER ? "/quota_refresh_user" : 
		"/quota_refresh_group");
	
	fd1 = open(sys_q_refresh, O_WRONLY);
	if (fd1 < 0) {
		fprintf(stderr, "can't open file %s: %s\n", sys_q_refresh, 
			strerror(errno));
		goto out;
	}
	
	if (write(fd1,(void*)"1", 1) != 1) {
		close(fd1);
		fprintf(stderr, "failed to write to %s: %s\n", 
			sys_q_refresh, strerror(errno));
		goto out;
	}
	close(fd1);
out:
	close(fd);
	close(metafs_fd);
	cleanup();
}

/**
 * main - Do everything
 * @argc: well, it's argc...
 * @argv: well, it's argv...
 *
 * Returns: exit status
 */

int
main(int argc, char *argv[])
{
	commandline_t comline;

	prog_name = argv[0];
	metafs_mounted = 0;

	memset(&comline, 0, sizeof(commandline_t));

	decode_arguments(argc, argv, &comline);

	switch (comline.operation) {
	case GQ_OP_LIST:
		do_list(&comline);
		break;

	case GQ_OP_GET:
		do_get(&comline);
		break;

	case GQ_OP_LIMIT:
	case GQ_OP_WARN:
		do_set(&comline);
		break;

	case GQ_OP_SYNC:
		do_sync(&comline);
		break;

	case GQ_OP_CHECK:
		do_sync(&comline);
		do_check(&comline);
		break;

	case GQ_OP_INIT:
		do_sync(&comline);
		do_quota_init(&comline);
		break;

	default:
		if (!comline.id_type) {
			comline.id_type = GQ_ID_USER;
			comline.id = geteuid();
		}
		do_get(&comline);
		break;
	}

	exit(EXIT_SUCCESS);
}
