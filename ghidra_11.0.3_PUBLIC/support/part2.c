// === zaphkiel_init @ 0022bd00 ===

void zaphkiel_init(void)

{
  (*(code *)PTR_zaphkiel_init_00233b20)();
  return;
}



// === __emutls_get_address @ 0022bd10 ===

void __emutls_get_address(void)

{
  (*(code *)PTR___emutls_get_address_00233b28)();
  return;
}



// === dl_iterate_phdr @ 0022bd20 ===

void dl_iterate_phdr(void)

{
  (*(code *)PTR_dl_iterate_phdr_00233b30)();
  return;
}



// === memmem @ 0022bd30 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void * memmem(void *__haystack,size_t __haystacklen,void *__needle,size_t __needlelen)

{
  void *pvVar1;
  
  pvVar1 = (void *)(*(code *)PTR_memmem_00233b38)();
  return pvVar1;
}



// === memset @ 0022bd40 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void * memset(void *__s,int __c,size_t __n)

{
  void *pvVar1;
  
  pvVar1 = (void *)(*(code *)PTR_memset_00233b40)(__s,__c);
  return pvVar1;
}



// === getrandom @ 0022bd50 ===

void getrandom(void)

{
  (*(code *)PTR_getrandom_00233b48)();
  return;
}



// === syscall @ 0022bd60 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

long syscall(long __sysno,...)

{
  long lVar1;
  
  lVar1 = (*(code *)PTR_syscall_00233b50)();
  return lVar1;
}



// === close @ 0022bd70 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int close(int __fd)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_close_00233b58)(__fd);
  return iVar1;
}



// === posix_memalign @ 0022bd80 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int posix_memalign(void **__memptr,size_t __alignment,size_t __size)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_posix_memalign_00233b60)((int)__memptr);
  return iVar1;
}



// === realloc @ 0022bd90 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void * realloc(void *__ptr,size_t __size)

{
  void *pvVar1;
  
  pvVar1 = (void *)(*(code *)PTR_realloc_00233b68)();
  return pvVar1;
}



// === dlsym @ 0022bda0 ===

void dlsym(void)

{
  (*(code *)PTR_dlsym_00233b70)();
  return;
}



// === dirfd @ 0022bdb0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int dirfd(DIR *__dirp)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_dirfd_00233b78)((int)__dirp);
  return iVar1;
}



// === closedir @ 0022bdc0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int closedir(DIR *__dirp)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_closedir_00233b80)((int)__dirp);
  return iVar1;
}



// === open @ 0022bdd0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int open(char *__file,int __oflag,...)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_open_00233b88)((int)__file,__oflag);
  return iVar1;
}



// === read @ 0022bde0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

ssize_t read(int __fd,void *__buf,size_t __nbytes)

{
  ssize_t sVar1;
  
  sVar1 = (*(code *)PTR_read_00233b90)(__fd);
  return sVar1;
}



// === fstat @ 0022bdf0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int fstat(int __fd,stat *__buf)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_fstat_00233b98)(__fd);
  return iVar1;
}



// === mkdir @ 0022be00 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int mkdir(char *__path,__mode_t __mode)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_mkdir_00233ba0)((int)__path,__mode);
  return iVar1;
}



// === munmap @ 0022be10 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int munmap(void *__addr,size_t __len)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_munmap_00233ba8)((int)__addr);
  return iVar1;
}



// === lseek64 @ 0022be20 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

__off64_t lseek64(int __fd,__off64_t __offset,int __whence)

{
  __off64_t _Var1;
  
  _Var1 = (*(code *)PTR_lseek64_00233bb0)(__fd,__offset,__whence);
  return _Var1;
}



// === mmap @ 0022be30 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void * mmap(void *__addr,size_t __len,int __prot,int __flags,int __fd,__off_t __offset)

{
  void *pvVar1;
  
  pvVar1 = (void *)(*(code *)PTR_mmap_00233bb8)(__addr,__len,__prot,__flags,__fd);
  return pvVar1;
}



// === realpath @ 0022be40 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

char * realpath(char *__name,char *__resolved)

{
  char *pcVar1;
  
  pcVar1 = (char *)(*(code *)PTR_realpath_00233bc0)();
  return pcVar1;
}



// === stat @ 0022be50 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int stat(char *__file,stat *__buf)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_stat_00233bc8)((int)__file);
  return iVar1;
}



// === getcwd @ 0022be60 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

char * getcwd(char *__buf,size_t __size)

{
  char *pcVar1;
  
  pcVar1 = (char *)(*(code *)PTR_getcwd_00233bd0)();
  return pcVar1;
}



// === calloc @ 0022be70 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void * calloc(size_t __nmemb,size_t __size)

{
  void *pvVar1;
  
  pvVar1 = (void *)(*(code *)PTR_calloc_00233bd8)();
  return pvVar1;
}



// === abort @ 0022be80 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void abort(void)

{
  (*(code *)PTR_abort_00233be0)();
  return;
}



// === write @ 0022be90 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

ssize_t write(int __fd,void *__buf,size_t __n)

{
  ssize_t sVar1;
  
  sVar1 = (*(code *)PTR_write_00233be8)(__fd);
  return sVar1;
}



// === getenv @ 0022bea0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

char * getenv(char *__name)

{
  char *pcVar1;
  
  pcVar1 = (char *)(*(code *)PTR_getenv_00233bf0)();
  return pcVar1;
}



// === readlink @ 0022beb0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

ssize_t readlink(char *__path,char *__buf,size_t __len)

{
  ssize_t sVar1;
  
  sVar1 = (*(code *)PTR_readlink_00233bf8)();
  return sVar1;
}



// === strerror_r @ 0022bec0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

char * strerror_r(int __errnum,char *__buf,size_t __buflen)

{
  char *pcVar1;
  
  pcVar1 = (char *)(*(code *)PTR_strerror_r_00233c00)(__errnum);
  return pcVar1;
}



// === gettid @ 0022bed0 ===

void gettid(void)

{
  (*(code *)PTR_gettid_00233c08)();
  return;
}



// === pthread_create @ 0022bee0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int pthread_create(pthread_t *__newthread,pthread_attr_t *__attr,__start_routine *__start_routine,
                  void *__arg)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_pthread_create_00233c10)((int)__newthread);
  return iVar1;
}



// === strcpy @ 0022bef0 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

char * strcpy(char *__dest,char *__src)

{
  char *pcVar1;
  
  pcVar1 = (char *)(*(code *)PTR_strcpy_00233c18)();
  return pcVar1;
}



// === pthread_once @ 0022bf00 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int pthread_once(pthread_once_t *__once_control,__init_routine *__init_routine)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_pthread_once_00233c20)((int)__once_control);
  return iVar1;
}



// === pthread_mutex_lock @ 0022bf10 ===

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

int pthread_mutex_lock(pthread_mutex_t *__mutex)

{
  int iVar1;
  
  iVar1 = (*(code *)PTR_pthread_mutex_lock_00233c28)((int)__mutex);
  return iVar1;
