/* clear_cache_Vox — Vox Manager PS5 Cache & Temp Cleaner.
 *
 * A PS5 payload built with the prospero payload SDK. When injected
 * (PS5 loader, e.g. port 9021) it walks the WebKit browser data
 * directories and removes cache/cookie/localstorage/tmp files.
 *
 * v2 fixes:
 *  - real browser data lives per user under /user/home/<uid>/webkit/shell
 *    (see vladimir-cucu/ps5-webkit-cache-remover and
 *    Storm21CH/PS5_Browser_appCache_remove); legacy webbrowser dirs are
 *    still cleaned on non-existent/older layouts.
 *  - notifications now use sceKernelSendNotificationRequest (plain text,
 *    no JSON, no libSceNotification dependency) so they actually show.
 *  - reports deleted file/dir counts and writes them to
 *    /data/clear_cache_vox.log for verification.
 */

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ps5/kernel.h>

typedef struct notify_request {
  char useless1[45];
  char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);

#define USERS_ROOT   "/user/home"
#define WEBKIT_DATA  "/webkit/shell"
#define RESULT_LOG   "/data/clear_cache_vox.log"

static unsigned long files_removed;
static unsigned long dirs_removed;

static void
notify(const char *message) {
  notify_request_t req;

  bzero(&req, sizeof(req));
  strncpy(req.message, message, sizeof(req.message) - 1);
  sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

static void
rmtree(const char *base) {
  char tmp_path[PATH_MAX + 1];
  struct stat st;
  struct dirent *dp;
  DIR *dir;

  if (!(dir = opendir(base))) {
    return;
  }

  while ((dp = readdir(dir))) {
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
      continue;
    }

    if (snprintf(tmp_path, sizeof(tmp_path), "%s/%s", base, dp->d_name)
        >= (int)sizeof(tmp_path)) {
      continue;
    }

    if (stat(tmp_path, &st)) {
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      rmtree(tmp_path);
      if (!rmdir(tmp_path)) {
        dirs_removed++;
      }
    } else {
      if (!unlink(tmp_path)) {
        files_removed++;
      }
    }
  }

  closedir(dir);
}

/* main cleanup: /user/home/<user>/webkit/shell for every user */
static void
clear_webkit_shell(void) {
  char user_path[PATH_MAX + 1];
  char shell_path[PATH_MAX + 1];
  struct stat st;
  struct dirent *dp;
  DIR *dir;

  if (!(dir = opendir(USERS_ROOT))) {
    return;
  }

  while ((dp = readdir(dir))) {
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
      continue;
    }

    if (snprintf(user_path, sizeof(user_path), "%s/%s", USERS_ROOT,
                 dp->d_name) >= (int)sizeof(user_path)) {
      continue;
    }

    if (stat(user_path, &st)) {
      continue;
    }
    if (!S_ISDIR(st.st_mode)) {
      continue;
    }

    if (snprintf(shell_path, sizeof(shell_path), "%s%s", user_path,
                 WEBKIT_DATA) >= (int)sizeof(shell_path)) {
      continue;
    }

    if (!stat(shell_path, &st) && S_ISDIR(st.st_mode)) {
      rmtree(shell_path);
    }
  }

  closedir(dir);
}

/* secondary cleanup: legacy webbrowser dirs (contents only) */
static void
clear_legacy_webbrowser(void) {
  const char *roots[] = {
      "/document/common/webbrowser",
      "/user/system/webkit/webbrowser",
  };
  const char *subdirs[] = {
      "cache", "cookies", "cookie", "databases", "localstorage", "tmp",
  };
  size_t r, s;
  char tmp_path[PATH_MAX + 1];

  for (r = 0; r < sizeof(roots) / sizeof(roots[0]); r++) {
    for (s = 0; s < sizeof(subdirs) / sizeof(subdirs[0]); s++) {
      if (snprintf(tmp_path, sizeof(tmp_path), "%s/%s", roots[r],
                   subdirs[s]) >= (int)sizeof(tmp_path)) {
        continue;
      }
      rmtree(tmp_path);
    }
  }
}

int
main(void) {
  pid_t pid = getpid();
  intptr_t rootdir = kernel_get_proc_rootdir(pid);
  char report[512];
  FILE *logf;

  files_removed = 0;
  dirs_removed = 0;

  notify("Vox Clean - cleaning started");

  kernel_set_proc_rootdir(pid, kernel_get_root_vnode());

  clear_webkit_shell();
  clear_legacy_webbrowser();

  kernel_set_proc_rootdir(pid, rootdir);

  snprintf(report, sizeof(report),
           "Vox Clean - removed %lu file(s) in %lu dir(s)",
           files_removed, dirs_removed);

  notify(report);

  if ((logf = fopen(RESULT_LOG, "w"))) {
    fprintf(logf, "%s\n", report);
    fclose(logf);
  }

  return EXIT_SUCCESS;
}