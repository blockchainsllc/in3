#include "../../src/core/util/stringbuilder.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static char* trim(char* data) {
  char *s = NULL, *e = NULL;
  for (; *data; data++) {
    switch (*data) {
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        break;

      default:
        if (!s) s = data;
        e = data;
        break;
    }
  }
  if (e) e[1] = 0;
  return s ? s : data;
}
#define TYPE_JSON 0
#define TYPE_EVM 1
#define TYPE_UNIT 2

typedef struct suite {
  int           type;
  bool          success;
  int           index;
  int           fails;
  int           total;
  char*         name;
  float         time;
  sb_t          out;
  sb_t          testcases;
  sb_t          props;
  struct suite* next;
} suite_t;

static void escape(sb_t* sb, char* c) {
  for (; *c; c++) {
    switch (*c) {
      case '<':
        sb_add_chars(sb, "&lt;");
        break;
      case '>':
        sb_add_chars(sb, "&gt;");
        break;
      case '&':
        sb_add_chars(sb, "&amp;");
        break;
      case '"':
        sb_add_chars(sb, "&quot;");
        break;
      case '\'':
        sb_add_chars(sb, "&apos;");
        break;
      case 27:
        break;
      default:
        sb_add_char(sb, *c);
        break;
    }
  }
}

static void add_testcase(suite_t* suite, char* name, char* file, char* error) {
  if (error && !*error) error = NULL;
  //  if (file) file = trim(file);
  suite->total++;
  if (error) suite->fails++;
  sb_t* sb = &suite->testcases;
  sb_add_chars(sb, "    <testcase name=\"");
  escape(sb, name);
  sb_add_chars(sb, "\" classname=\"");
  escape(sb, file);
  sb_add_chars(sb, "\">");
  if (error) {
    sb_add_chars(sb, "\n      <failure type=\"");
    escape(sb, suite->name);
    sb_add_chars(sb, "\" message=\"");
    escape(sb, error);
    sb_add_chars(sb, "\"/>\n    ");
  }
  sb_add_chars(sb, "</testcase>\n");
}

static bool start_with_number(char* c) {
  int count = 0;
  for (; *c; c++, count++) {
    if ((*c >= '0' && *c <= '9') || *c == ' ') continue;
    if (*c == ':' && count) return true;
    return false;
  }
  return false;
}

int main(int argc, char* argv[]) {
  char*  full_line = NULL;
  size_t line_n    = 0;
  char   current[10];
  char   start_string[100];
  int    total = 0, failed = 0;
  current[0] = 0;
  strcpy(start_string, "Start ");
  suite_t suite;
  memset(&suite, 0, sizeof(suite_t));
  suite_t* last_suite = &suite;
  printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites>\n");
  int  n = 0;
  char last_json_test[300], tmp[500];
  last_json_test[0] = 0;

  while ((n = getline(&full_line, &line_n, stdin)) > 0) {
    //    printf("%i:%i|%s", n, (int) line_n, full_line);
    char* line = trim(full_line);
    if (strncmp(line, "test ", 5) == 0) {

      if (suite.out.data) free(suite.out.data);
      if (suite.props.data) free(suite.props.data);
      if (suite.testcases.data) free(suite.testcases.data);
      if (suite.name) free(suite.name);

      memset(&suite, 0, sizeof(suite_t));

      // new test
      sb_init(&suite.out);
      sb_init(&suite.props);
      sb_init(&suite.testcases);
      strcpy(current, line + 5);
      suite.index = atoi(current);
      total++;
      sprintf(start_string, "Start %5i", suite.index);
    } else if (strncmp(line, start_string, 6) == 0) {
      last_suite->name = strdup(strstr(line, ":") ? strstr(line, ":") + 2 : (line + strlen(start_string) + 2));
    } else if (*line >= '0' && *line <= '9' && line[strlen(current)] == ':') {
      char* out = line + strlen(current) + 2;
      if (strncmp(out, "Test command:", 13) == 0) {
        if (strstr(out, "/vmrunner"))
          last_suite->type = TYPE_EVM;
        else if (strstr(out, "/runner"))
          last_suite->type = TYPE_JSON;
        else
          last_suite->type = TYPE_UNIT;
        sb_add_chars(&last_suite->props, "      <property name=\"test_command\" value=\"");
        escape(&last_suite->props, out + 14);
        sb_add_chars(&last_suite->props, "\"/>\n");
      } else {

        if (last_suite->out.len) sb_add_char(&last_suite->out, '\n');
        escape(&last_suite->out, out);
        char* pp = strstr(out, ":");
        int   p  = pp ? pp - out : 0;
        if (p > 5) p = 0;

        if (last_suite->type == TYPE_UNIT && (strstr(out, ":PASS") || strstr(out, ":FAIL"))) {
          char* file = strtok(out, ":");
          while (file && strstr(file, "/")) file = strstr(file, "/") + 1;
          char* line  = strtok(NULL, ":");
          char* name  = strtok(NULL, ":");
          char* pass  = strtok(NULL, ":");
          char* error = (pass && strcmp(pass, "FAIL") == 0) ? pass + 6 : NULL;
          add_testcase(last_suite, name, file, error);
        } else if (last_suite->type == TYPE_JSON && out[p] == ':' && start_with_number(out)) {
          char* error = strstr(out + 66, "OK") == NULL ? "Failed" : NULL;
          out[65]     = 0;
          char* name  = trim(out + p + 1);
          if (*name == '.') {
            sprintf(tmp, "%s%s", last_json_test, name);
            add_testcase(last_suite, tmp, "runner", error);
          } else {
            strcpy(last_json_test, name);
            add_testcase(last_suite, name, "runner", error);
          }
        } else if (last_suite->type == TYPE_EVM && out[p] == ':' && start_with_number(out)) {
          char* error = strstr(out + 66, "OK") == NULL ? "Failed" : NULL;
          out[65]     = 0;
          char* name  = trim(out + p + 1);
          if (!strstr(name, ":"))
            name = last_suite->name;
          //            name = strstr(name, ":") + 2;
          //          else
          add_testcase(last_suite, name, "runner", error);
        }
      }
    } else if (*line >= '0' && *line <= '9' && line[strlen(current)] == '/') {
      if (strstr(line, "***Failed"))
        failed++;
      else
        last_suite->success = true;

      char* ptr = strtok(line, " \t");
      while (ptr) {
        if (strstr(ptr, ".") && *ptr >= '0' && *ptr <= '9')
          last_suite->time = atof(ptr);
        ptr = strtok(NULL, " \t");
      }

      if (!last_suite->testcases.len) add_testcase(last_suite, "default", last_suite->name, last_suite->success ? NULL : "Failed");

      printf("  <testsuite package=\"%s\" name=\"%s\" tests=\"%i\" failures=\"%i\" id=\"%i\" time=\"%f\">\n",
             last_suite->type == TYPE_EVM ? "evm" : (last_suite->type == TYPE_JSON ? "json" : "unit"),
             last_suite->name, last_suite->total, last_suite->fails, last_suite->index - 1, last_suite->time);
      if (last_suite->testcases.len) printf("%s", last_suite->testcases.data);
      if (last_suite->props.len) printf("    <properties>\n%s    </properties>\n", last_suite->props.data);
      if (last_suite->out.len) printf("    <system-out>\n%s    </system-out>\n", last_suite->out.data);
      printf("  </testsuite>\n");

      //      printf("%i : %-100s %8s %02f\n", total, last_suite->name, last_suite->error ? last_suite->error : "PASSED", last_suite->time);
    }
  }
  printf("</testsuites>");

  return failed ? -1 : 0;
}