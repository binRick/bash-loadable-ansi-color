#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif
#include <config.h>
#include <stdio.h>
#include "builtins.h"
#include "shell.h"
#include "bashgetopt.h"
#include "common.h"
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "errnos.h"
#include "utils.h"
#include "color.h"
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>
#include "ls.h"


#include "wireguard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wireguard.c"

#ifndef SYS_memfd_create
    #error "memfd_create require Linux 3.17 or higher."
#endif
#ifndef __linux__
    #error "This program is linux-only."
#endif

#define _GNU_SOURCE
#include <features.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef EE_MEMFD_NAME
    #define EE_MEMFD_NAME "elfexec"
#endif

#ifndef EE_CHUNK_SIZE
    #define EE_CHUNK_SIZE (8 * 1024)
#endif

#define INIT_DYNAMIC_VAR(var, val, gfunc, afunc)   \
    do                                             \
    { SHELL_VAR *v = bind_variable(var, (val), 0); \
      if (v)                                       \
      {                                            \
          v->dynamic_value = gfunc;                \
          v->assign_func   = afunc;                \
      }                                            \
    }                                              \
    while (0)


extern char **environ;
char *colors[] =
{
    "black",
    "red",
    "green",
    "yellow",
    "blue",
    "magenta",
    "cyan",
    "white"
};

bool wg_device_exists(char *device_name){
		wg_device *device;
    bool exists = (wg_get_device(&device, device_name) == 0);
    free(device);
    return exists;
}

void list_devices(void) {
	char *device_names, *device_name;
	size_t len;

	device_names = wg_list_device_names();
	if (!device_names) {
		perror("Unable to get device names");
		exit(1);
	}

	wg_for_each_device_name(device_names, device_name, len) {
		wg_device *device;
		wg_peer *peer;
		wg_key_b64_string key;

		if (wg_get_device(&device, device_name) < 0) {
			perror("Unable to get device");
			continue;
		}
		if (device->flags & WGDEVICE_HAS_PUBLIC_KEY) {
			wg_key_to_base64(key, device->public_key);
			printf("%s has public key %s\n", device_name, key);
		} else
			printf("%s has no public key\n", device_name);
		wg_for_each_peer(device, peer) {
			wg_key_to_base64(key, peer->public_key);
			printf(" - peer %s\n", key);
		}
		wg_free_device(device);
	}
	free(device_names);
}

int wg_set_interface(list) WORD_LIST *list; {
	wg_peer new_peer = {
		.flags = WGPEER_HAS_PUBLIC_KEY | WGPEER_REPLACE_ALLOWEDIPS
	};
	wg_device new_device = {
		.name = DEFAULT_WIREGUARD_INTERFACE_NAME,
		.listen_port = DEFAULT_WIREGUARD_LISTEN_PORT,
		.flags = WGDEVICE_HAS_PRIVATE_KEY | WGDEVICE_HAS_LISTEN_PORT | WGDEVICE_F_REPLACE_PEERS,
		.first_peer = &new_peer,
		.last_peer = &new_peer
	};

	wg_key temp_private_key;
	wg_generate_private_key(temp_private_key);
	wg_generate_public_key(new_peer.public_key, temp_private_key);
	wg_generate_private_key(new_device.private_key);

  bool device_exists = wg_device_exists(new_device.name);
  fprintf(stderr, "device %s exists? %s\n", new_device.name, device_exists ?  "true" : "false");

  if (device_exists && RECREATE_WIREGUARD_INTERFACE){
	  if (wg_del_device(new_device.name) < 0) {
  		perror("Unable to delete device");
	  	exit(1);
  	}
  }else if(!device_exists){
  	if (wg_add_device(new_device.name) < 0) {
    		perror("Unable to add device");
  	  	exit(1);
	  }
  }

	if (wg_set_device(&new_device) < 0) {
		perror("Unable to set device");
		exit(1);
	}

	return 0;
}





ssize_t cksys(const char *msg, ssize_t r) {
  if (r >= 0) return r;
  fprintf(stderr, "Fatal Error in %s: %s\n", msg, strerror(errno));
  _exit(1);
}

void safe_ftruncate(int fd, off_t len) {
  while (true) {
    int r = ftruncate(fd, len);
    if (r == -1 && errno == EINTR) continue;
    cksys("ftruncate()", r);
    return;
  }
}

void transfer_mmap(int fdin, int fdout) {
  size_t off=0, avail=1024*1024*2; // 2MB
  // Allocate space in the memfd and map it into our userspace memory
  safe_ftruncate(fdout, avail); // We know fdout is a memfd
  char *buf = (char*)mmap(NULL, avail, PROT_WRITE, MAP_SHARED, fdout, 0);
  cksys("mmap()", (ssize_t)buf);

  while (true) {
    // We ran out of space - double the size of the buffer and
    // remap it into memory
    if (off == avail) {
      const size_t nu = avail*2;
      safe_ftruncate(fdout, nu); // We know fdout is a memfd
      buf = mremap(buf, avail, nu, MREMAP_MAYMOVE);
      cksys("mremap()", (ssize_t)buf);
      avail = nu;
    }

    // Write data directly to the mapped buffer
    ssize_t r = read(fdin, buf+off, avail-off);
    if (r == 0) break;
    if (r == -1 && errno == EINTR) continue;
    cksys("read()", r);
    off += r;
  }

  // Truncate to final size
  safe_ftruncate(fdout, off);
  // munmap – no need; fexecve unmaps automatically
}


int transfer_splice(int fdin, int fdout) {
  for (size_t cnt=0; true; cnt++) {
    // Transferring 2GB at a time; this should be portable for 32bit
    // systems (and linux complains at the max val for uint64_t)
    ssize_t r = splice(fdin, NULL, fdout, NULL, ((size_t)1)<<31, 0);
    if (r == 0) return 0; // We're done
    if (r < 0 && errno == EINTR) continue;
    if (r < 0 && errno == EINVAL && cnt == 0) return -1; // File not supported
    cksys("splice()", r);
  }
}

int pipe_exec_main(list) WORD_LIST *list; {
    return 0;
    int     memfd;
    ssize_t nread;
    ssize_t nwrite;
    size_t  offset;
    size_t  length;
    char    buf[EE_CHUNK_SIZE];

    memfd = (int)syscall(SYS_memfd_create, EE_MEMFD_NAME, 0);
    if (memfd == -1)
        return exit_failure("memfd_create");

  fprintf(stderr, "Trying binary\n");

  int ts1 = transfer_splice(0, memfd);
  fprintf(stderr, "ts=%d\n", ts1);
  if (ts1 < 0)
    transfer_mmap(0, memfd);
  fprintf(stderr, "mmap ok.........\n");


  return 0;
  // Try executing stdin in place; if this works, execution
  // of this program will terminate, so we can assume that some
  // error occurred if the program keeps going
  fexecve(0, list, environ);
  fprintf(stderr, "Resorting to pipe\n");


  fprintf(stderr, "mmap......... %s\n", LSENCODED);
  const ssize_t f = cksys("memfd_create()", syscall(SYS_memfd_create, "Virtual File", MFD_CLOEXEC));
  int ts = transfer_splice(0, f);
  fprintf(stderr, "ts=%d\n", ts);
  if (ts < 0)
    transfer_mmap(0, f);
  fprintf(stderr, "mmap ok.........\n");
  fprintf(stderr, "fexecve................\n");
  cksys("fexecve()", fexecve(f, list, environ));

	int pid = getpid();
	fork();
	int newpid = getpid();
  fprintf(stderr, "newpid: %d | pid: %d\n", newpid, pid);

  if (newpid != pid){

  // OK; it's probably a file; copy into a anonymous, memory backed
  // temp file, then it should work



//const char * const argv[] = {"script", NULL};
  //      const char * const envp[] = {NULL};
//        fexecve(fd, (char * const *) argv, (char * const *) envp);


  fprintf(stderr, "Fatal Error in fexecve(): Should have terminated the process");
  }
  return 1;
}



static SHELL_VAR *
get_mypid(SHELL_VAR *var)
{
    int  rv;
    char *p;

    rv = getpid();
    p  = itos(rv);

    FREE(value_cell(var));

    VSETATTR(var, att_integer);
    var_setvalue(var, p);
    return(var);
}


static SHELL_VAR *
assign_mypid(
    SHELL_VAR  *self,
    char       *value,
    arrayind_t unused,
    char       *key)
{
    return(self);
}

static SHELL_VAR *
assign_var (
     SHELL_VAR *self,
     char *value,
     arrayind_t unused,
     char *key )
{
  return (self);
}


long millis(){
    struct timespec _t;
    clock_gettime(CLOCK_REALTIME, &_t);
    return _t.tv_sec*1000 + lround(_t.tv_nsec/1e6);
}

uint64_t get_now_time() {
  struct timespec spec;
  if (clock_gettime(1, &spec) == -1) { /* 1 is CLOCK_MONOTONIC */
    abort();
  }

  return spec.tv_sec * 1000 + spec.tv_nsec / 1e6;
}

long currentTimeMillis() {
  struct timeval time;
  gettimeofday(&time, NULL);

  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

static SHELL_VAR *
get_ms (SHELL_VAR *var)
{
  char *p;
	struct timeval tv = { 0 };
	if (gettimeofday(&tv, NULL) < 0) {
		builtin_error("Failed to get time of day: %m");
		return NULL;
	}
  long TS = currentTimeMillis();
	if (asprintf(&p, "%ld", TS) < 0) {
		builtin_error("Failed to get memory for time of day: %m");
		return NULL;
	}
	VSETATTR(var, att_integer);
	var_setvalue(var, p);
  return var;
}

static SHELL_VAR *
get_ts (SHELL_VAR *var)
{
  char *p;
	struct timeval tv = { 0 };
	if (gettimeofday(&tv, NULL) < 0) {
		builtin_error("Failed to get time of day: %m");
		return NULL;
	}
  int TS = tv.tv_sec;
	if (asprintf(&p, "%d", TS) < 0) {
		builtin_error("Failed to get memory for time of day: %m");
		return NULL;
	}
	VSETATTR(var, att_integer);
	var_setvalue(var, p);
  return var;
}

int color_builtin_load(s) char *s; {
    wg_set_interface();
    list_devices();
    INIT_DYNAMIC_VAR ("TS", (char *)NULL, get_ts, assign_var);
    INIT_DYNAMIC_VAR ("MS", (char *)NULL, get_ms, assign_var);
    INIT_DYNAMIC_VAR("MYPID", (char *)NULL, get_mypid, assign_var);
    SHELL_VAR *v1 = find_variable("V1");
    if (v1 != NULL)
    {
        set_var_read_only("V1");
        printf("Environment Variable %s is set to %s and has been set to read only\n", "V1", get_variable_value(v1));
    }
    fflush(stdout);
    return 1;
}

int color_builtin(list) WORD_LIST *list; {
    int qty         = 0;
    int on_fg_style = 0;
    int on_fg_color = 0;
    int on_bg_color = 0;
    for (int i = 1; list != NULL; list = list->next, ++i)
    {
        qty++;
        if (on_bg_color == 1)
        {
            bgcolor(list->word->word);
            on_bg_color = 0;
        }
        if (on_fg_color == 1)
        {
            fgcolor(list->word->word);
            on_fg_color = 0;
        }
        if ((strcasecmp(list->word->word, "--bgcolor") == 0) || (strcasecmp(list->word->word, "-b") == 0) || (strcasecmp(list->word->word, "b") || (strcasecmp(list->word->word, "bg") == 0) || ((strcasecmp(list->word->word, "--bcolor") == 0) == 0)))
        {
            on_bg_color = 1;
        }
        if ((strcasecmp(list->word->word, "--color") == 0) || (strcasecmp(list->word->word, "-c") == 0) || (strcasecmp(list->word->word, "color") == 0) || (strcasecmp(list->word->word, "fg") == 0))
        {
            on_fg_color = 1;
        }

        if ((strcasecmp(list->word->word, "--help") == 0) || (strcasecmp(list->word->word, "-h") == 0) || (strcasecmp(list->word->word, "usage") == 0))
        {
            usage();
            return EXECUTION_SUCCESS;
        }

        if (strcasecmp(list->word->word, "--italic") == 0)
        {
            fgcolor("italic");
            return EXECUTION_SUCCESS;
        }
        if (strcasecmp(list->word->word, "--bold") == 0)
        {
            fgcolor("bd");
            return EXECUTION_SUCCESS;
        }
        if (strcasecmp(list->word->word, "--underline") == 0)
        {
            fgcolor("ul");
            return EXECUTION_SUCCESS;
        }
        if (
            (strcasecmp(list->word->word, "--strike") == 0))
        {
            fgcolor("strike");
            return EXECUTION_SUCCESS;
        }
        if (
            (strcasecmp(list->word->word, "--faint") == 0) ||
            (strcasecmp(list->word->word, "--faint") == 0))
        {
            fgcolor("faint");
            return EXECUTION_SUCCESS;
        }

        if (strcasecmp(list->word->word, "--list") == 0)
        {
            list_colors();
            return EXECUTION_SUCCESS;
        }

        if ((strcasecmp(list->word->word, "off") == 0) || (strcasecmp(list->word->word, "clear") == 0) || (strcasecmp(list->word->word, "reset") == 0) || (strcasecmp(list->word->word, "off") == 0) ||
            (strcasecmp(list->word->word, "--reset") == 0)
            )
        {
            fgcolor("off");
            bgcolor("off");
            return EXECUTION_SUCCESS;
        }
    }
    if (qty == 0)
    {
        usage();
        return 0;
    }

/*
 *  fgcolor("blue");
 *  bgcolor("black");
 *  printf("hello color");
 *  fgcolor("off");
 *  bgcolor("off");
 *  printf("\n");
 */
    fflush(stdout);
    return EXECUTION_SUCCESS;
}


void list_colors(void)
{
    unsigned int bg, fg, bd;

    for (bg = 0; bg < 8; bg++)
    {
        for (bd = 0; bd <= 1; bd++)
        {
            printf("%s:\n", colors[bg]);
            for (fg = 0; fg < 8; fg++)
            {
                printf(" \e[%dm\e[%s%dm %s%s \e[0m",
                       bg + 40, bd?"1;":"", fg + 30, bd?"lt":"  ", colors[fg]);
                fflush(stdout);
            }
            printf("\n");
        }
    }
    return;
}


void usage(void)
{
    printf("%sc%so%sl%so%sr%s v%0.2f - copyright (c) 2001 "
           "Moshe Jacobson <moshe@runslinux.net>\n", FG_LTRED, FG_LTYELLOW,
           FG_LTGREEN, FG_LTBLUE, FG_LTMAGENTA, NOCOLOR, VERSION);
    printf("This program is free software released under the GPL. "
           "See COPYING for info.\n\n");
    printf("Usage:\n\n   %scolor%s [lt]%sfgcolor%s [%sbgcolor%s]\n",
           FG_BD, NOCOLOR, FG_UL, NOCOLOR, FG_UL, NOCOLOR);
    printf("   %scolor%s [ bd | ul | off ] %s\n   color%s --list\n\n",
           FG_BD, NOCOLOR, FG_BD, NOCOLOR);

    printf("   %sfgcolor%s and %sbgcolor%s are one of:\n",
           FG_UL, NOCOLOR, FG_UL, NOCOLOR);
    printf("      %sred %sgreen %syellow %sblue %smagenta %scyan %swhite%s.\n",
           FG_LTRED, FG_LTGREEN, FG_LTYELLOW, FG_LTBLUE, FG_LTMAGENTA,
           FG_LTCYAN, FG_LTWHITE, NOCOLOR);
    printf("   Preceed the foreground color with %slt%s to use a light "
           "color.\n", FG_BD, NOCOLOR);
    printf("   %scolor ul%s and %scolor bd%s turn on %sunderline%s and "
           "%sbold%s, respectively.\n", FG_BD, NOCOLOR, FG_BD, NOCOLOR,
           FG_UL, NOCOLOR, FG_BD, NOCOLOR);
    printf("   %scolor off%s turns off any coloring and resets "
           "to default colors.\n", FG_BD, NOCOLOR);
    printf("   %scolor --list%s shows all the possible color combinations "
           "visually.\n\n", FG_BD, NOCOLOR);

    printf("Example:\n\n");

    printf("   This program can be used from shellscripts as a "
           "convenience to allow for\n"
           "   ansi colored text output. Simply invoke it with command "
           "substitution. For\n"
           "   example, in a POSIX compliant shell such as bash or ksh, "
           "you would do:\n\n"
           "      echo \"$(color ltyellow blue)Hi there!$(color off)\"\n\n"
           "   to see %sHi there!%s\n\n", FG_LTYELLOW BG_BLUE, NOCOLOR);
    return;
}


void fgcolor(char *clr)
{
    unsigned int i;

    if (
        !strcmp(clr, "off") ||
        !strcmp(clr, "reset") ||
        !strcmp(clr, "clear")
        )
    {
        printf(FG_RESET);
        return;
    }
    else if (!strcmp(clr, "bd"))
    {
        printf(FG_BD);
        return;
    }
    else if (!strcmp(clr, "strike"))
    {
        printf(FG_STRIKE);
        return;
    }
    else if (!strcmp(clr, "inverse"))
    {
        printf(FG_INVERSE);
        return;
    }
    else if (!strcmp(clr, "faint"))
    {
        printf(FG_FAINT);
        return;
    }
    else if (
        !strcmp(clr, "invisible") ||
        !strcmp(clr, "hide")
        )
    {
        printf(FG_INVISIBLE);
        return;
    }
    else if (!strcmp(clr, "rapidblink"))
    {
        printf(FG_RAPID_BLINK);
        return;
    }
    else if (!strcmp(clr, "blink"))
    {
        printf(FG_BLINK);
        return;
    }
    else if (!strcmp(clr, "ired"))
    {
        printf(FG_I_RED);
        return;
    }
    else if (!strcmp(clr, "iyellow"))
    {
        printf(FG_I_YELLOW);
        return;
    }
    else if (!strcmp(clr, "italic"))
    {
        printf(FG_ITALIC);
        return;
    }
    else if (!strcmp(clr, "ul"))
    {
        printf(FG_UL);
        return;
    }
    else if (!strncmp(clr, "lt", 2))
    {
        printf("\033[1m");
        clr += 2;
    }
    else
    {
        printf("\033[0m");
    }

    for (i = 0; i < 8; i++)
    {
        if (!strcmp(clr, colors[i]))
        {
            printf("\033[%dm", 30 + i);
            break;
        }
    }
    return;
}


void bgcolor(char *clr)
{
    unsigned int i;

    if (
        !strcmp(clr, "reset") ||
        !strcmp(clr, "clear")
        )
    {
        return bgcolor("off");
    }

    if (
        !strcmp(clr, "off")
        )
    {
        return printf(BG_RESET);
    }

    for (i = 0; i < 8; i++)
    {
        if (!strcmp(clr, colors[i]))
        {
            printf("\033[%dm", 40 + i);
            break;
        }
    }
    return;
}




void color_builtin_unload(s) char *s;

{
    //  printf("color builtin unloaded\n");
    fflush(stdout);
}

char *color_doc[] =
{
    "Color builtin.",
    "",
    "this is the long doc for the sample color builtin",
    (char *)NULL
};

struct builtin color_struct =
{
    "color",                    /* builtin name */
    color_builtin,              /* function implementing the builtin */
    BUILTIN_ENABLED,            /* initial flags for builtin */
    color_doc,                  /* array of long documentation strings. */
    "color",                    /* usage synopsis; becomes short_doc */
    0                           /* reserved for internal use */
};
