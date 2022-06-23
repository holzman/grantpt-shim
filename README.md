# grantpt - shim library for glibc < 2.23
This is a backport of grantpt from glibc-2.28 for earlier glibc versions (particularly glibc-2.17 that ships with RHEL7 and clones).  This is needed to allow opening ptys inside user namespaces with older glibc.

## Build

```gcc -shared -fPIC -o grantpt.so itoa.c ptsname-internal.c grantpt.c ```

## Use

```LD_PRELOAD=./grantpt.so unshare -U python -c 'import os; os.openpty()'```
