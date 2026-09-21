/* clear_cache_Vox — Vox Manager PS5 Cache & Temp Cleaner.
 *
 * A PS5 payload built with the prospero payload SDK. When injected
 * (PS5 loader, e.g. port 9021) it walks the WebKit browser data
 * directories under /document/common/webbrowser and
 * /user/system/webkit/webbrowser, deletes cache/cookie/localstorage
 * files and shows a TV notification with the number of removed files.
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

#define SCE_NOTIFICATION_LOCAL_USER_ID_SYSTEM 0xFE

int sceNotificationSend(int userId, bool isLogged, const char *payload);

static const char *WEBBROWSER_ROOTS[] = {
    "/document/common/webbrowser",
    "/user/system/webkit/webbrowser",
};

static const char *TARGET_SUBDIRS[] = {
    "cache",
    "cookies",
    "cookie",
    "databases",
    "localstorage",
    "tmp",
};

static const char CLEAN_TOAST[] =
  "{\n"
  "  \"rawData\": {\n"
  "    \"viewTemplateType\": \"EEED\",\n"
  "    \"useCaseId\": \"IDC\",\n"
  "    \"priority\": 80,\n"
  "    \"viewData\": {\n"
  "      \"message\": {\n"
  "        \"body\": \"Vox Clean\"\n"
  "      },\n"
  "      \"subMessage\": {\n"
  "        \"body\": \"PS5 cache & temp data cleared\"\n"
  "      }\n"
  "    }\n"
  "  }\n"
  "}";

static unsigned long files_removed;
static unsigned long dirs_removed;

static int
rmtree(const char *base) {
  char tmp_path[PATH_MAX + 1];
  struct stat st;
  struct dirent *dp;
  DIR *dir;

  if (!(dir = opendir(base))) {
    return -1;
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
    } else if (S_ISREG(st.st_mode)) {
      if (!unlink(tmp_path)) {
        files_removed++;
      }
    } else {
      if (!unlink(tmp_path)) {
        files_removed++;
      }
    }
  }
  closedir(dir);
  return 0;
}

static int
clear_webbrowser_cache(void) {
  size_t r, s;
  char tmp_path[PATH_MAX + 1];

  for (r = 0; r < sizeof(WEBBROWSER_ROOTS) / sizeof(WEBBROWSER_ROOTS[0]);
       r++) {
    for (s = 0; s < sizeof(TARGET_SUBDIRS) / sizeof(TARGET_SUBDIRS[0]); s++) {
      if (snprintf(tmp_path, sizeof(tmp_path), "%s/%s", WEBBROWSER_ROOTS[r],
                   TARGET_SUBDIRS[s]) >= (int)sizeof(tmp_path)) {
        continue;
      }
      printf("[Vox Clean] scanning %s\n", tmp_path);
      if (rmtree(tmp_path) == 0) {
        /* keep the top level dir, it is owned by the system */
        rmdir(tmp_path);
      }
    }
  }
  return 0;
}

int
main(void) {
  pid_t pid = getpid();
  intptr_t rootdir = kernel_get_proc_rootdir(pid);
  int rc;

  kernel_set_proc_rootdir(pid, kernel_get_root_vnode());

  files_removed = 0;
  dirs_removed = 0;

  printf("%s", "Vox Manager PS5 Cache Cleaner Loaded...\n");
  rc = clear_webbrowser_cache();
  printf("[Vox Clean] removed %lu file(s) in %lu dir(s)\n", files_removed,
         dirs_removed);

  kernel_set_proc_rootdir(pid, rootdir);

  if (rc) {
    return EXIT_FAILURE;
  }

  sceNotificationSend(SCE_NOTIFICATION_LOCAL_USER_ID_SYSTEM, true,
                      CLEAN_TOAST);
  return EXIT_SUCCESS;
}