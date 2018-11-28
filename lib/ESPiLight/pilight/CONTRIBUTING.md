Rules for contributing to pilight
=================================
First of all, thanks for making this project better. Without your help, pilight wouldn't have come to what it is now!

**BUT**, be aware that contributing to a big project shouldn't be considered a playground. It also isn't meant for you to learn `git`.
So if you don't know what you are doing then don't do it. Instead open a [forum topic](http://forum.pilight.org), point to your code and ask for help.

Checklist
---------
When you contribute to pilight make sure to follow this checklist:

1. Is your pull request directed at the development branch?
*pilight doesn't accept contributions to the master branch because this branch reflects the latest stable code.*
2. Did you read the [development](http://manual.pilight.org/development/) pages of pilight?
*Even if you think you know all, please check and double check. There are always small syntax changes. Also check similar already existing modules for syntax.*
3. Keep the coding style in sync with that of pilight (see below).
4. First merge with the latest development code.
5. Make a different branch for each new feature you want to commit.
6. Test how pull-requests work on your own test repositories.
7. Make sure your pull-request contains [one single commit](http://eli.thegreenplace.net/2014/02/19/squashing-github-pull-requests-into-a-single-commit).
8. Open a pull-request when you indeed want to contribute and follow-up on our comments. If you don't want to implement our requested changes after reviewing your pull-request, don't bother opening one.
9. Re-read this file before every pull-request, because it will be updated regularly.
10. Don't forget to enjoy the appreciation of the end user!

Coding style
-----
- No unnecessary spaces
```
if ( 1 == 1 )
{
...
}
```
should become:
```
if(1 == 1) {
...
}
```
- Don't inline variables:
```
int x                  = 0;
int long_variable_name = 0;
int a                  = 0;
```
but use
```
int x =	0;
int long_variable_name = 0;
int a = 0;
```
but preferable use for around max 50 characters:
```
int x = 0, long_variable_name = 0, a = 0;
```
- Variable defining order.
First start with `struct`, then specials types (`*_t`), then `char`, then `double` / `float` and end with `int`.
```
	struct protocol_threads_t *node = (struct protocol_threads_t *)param;
	struct JsonNode *json = (struct JsonNode *)node->param;
	struct JsonNode *jid = NULL;
	struct JsonNode *jchild = NULL;
	time_t time;
	char *tmp = NULL;
	double itmp = 0.0;
	int id = 0, state = 0, nstate = 0;
```
- Initialize your variables.
```
char *a = NULL;
char a[10];
memset(&a, 0, 10);
double a = 0.0;
int a = 0;
```
- User the `static` keyword for all variables and function only use in the single C file your module consists of.
- Always use tabs instead of spaces for inline markup.
