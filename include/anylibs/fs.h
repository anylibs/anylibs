#ifndef ANYLIBS_FS_H
#define ANYLIBS_FS_H

#include <time.h>

typedef struct anylibs_fs_file_t anylibs_fs_file_t;

typedef enum anylibs_fs_file_type_t {
  C_FS_FILE_TYPE_unknown,
  C_FS_FILE_TYPE_file,
  C_FS_FILE_TYPE_dir,
  C_FS_FILE_TYPE_symlink,
} anylibs_fs_file_type_t;

typedef enum anylibs_fs_file_permission_t {
  C_FS_FILE_PERMISSION_unknown,
  C_FS_FILE_PERMISSION_read_only,
  C_FS_FILE_PERMISSION_read_write,
} anylibs_fs_file_permission_t;

typedef struct anylibs_fs_metadata_t {
  anylibs_fs_file_type_t ftype;
  size_t fsize;
  anylibs_fs_file_permission_t fperm;
  time_t last_modified;
  time_t last_accessed;
  time_t created_time;
} anylibs_fs_metadata_t;

typedef struct anylibs_fs_path_t anylibs_fs_path_t;
typedef struct anylibs_fs_pathbuf_t anylibs_fs_pathbuf_t;
typedef struct anylibs_fs_pathref_t anylibs_fs_pathref_t;

struct anylibs_fs_path_t {
  char const* data;
  size_t size;
};

struct anylibs_fs_pathbuf_t {
  char* data;
  size_t size;
  size_t capacity;
};

struct anylibs_fs_pathref_t {
  char* data;
  size_t size;
};

typedef struct anylibs_fs_iter_t {
  anylibs_fs_pathbuf_t* pathbuf;
  size_t old_len;
  void* fptr;
} anylibs_fs_iter_t;

/// Create new `anylibs_fs_path_t`
///
/// # Example
///
/// ```
/// const char* raw = "/folder/file";
/// anylibs_fs_path_t path = anylibs_fs_path_create(raw, strlen(path));
/// ```
anylibs_fs_path_t anylibs_fs_path_create(char const path[], size_t path_size);

/// Create new `anylibs_fs_pathbuf_t`, same like `anylibs_fs_path_t` but the `data` could be changed withing its `capacity`
///
/// # Example
///
/// ```
/// char raw[] = "/folder/file";
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(raw, strlen(raw), strlen(raw) + 1); // `+ 1` for the trailing zero
/// ```
anylibs_fs_pathbuf_t anylibs_fs_pathbuf_create(char pathbuf[], size_t pathbuf_size, size_t pathbuf_capacity);

/// Open a file and return `anylibs_fs_file_t*` or return `NULL` on error.
/// \note On Windows it will open with utf-8 and binary modes.
///
/// `mode` is the same as mode parameter for `fopen`:
/// ## Text mode:
/// These modes handle the file as a text file, which may process newline characters differently (e.g., converting `\n` to `\r\n` on Windows).
/// | Mode   | Description |
/// | :--    | :--         |
/// | `"r"`  |	Open for reading. The file must exist. |
/// | `"w"`  |	Open for writing. Creates a new file or truncates an existing file. |
/// | `"a"`  |	Open for appending. Creates a new file if it doesn't exist. |
/// | `"r+"` |	Open for reading and writing. The file must exist. |
/// | `"w+"` |	Open for reading and writing. Creates a new file or truncates an existing file. |
/// | `"a+"` |	Open for reading and appending. Creates a new file if it doesn't exist. |
///
/// ## Binary mode:
/// These modes treat the file as binary, where no newline character conversion occurs. Add a `"b"` to the mode to open the file in binary mode.
/// | Mode    | Description |
/// | :--     | :--         |
/// | `"rb"`  | 	Open for reading in binary mode. The file must exist. |
/// | `"wb"`  | 	Open for writing in binary mode. Creates a new file or truncates an existing file. |
/// | `"ab"`  | 	Open for appending in binary mode. Creates a new file if it doesn't exist. |
/// | `"r+b"` |	Open for reading and writing in binary mode. The file must exist. |
/// | `"w+b"` |	Open for reading and writing in binary mode. Creates a new file or truncates an existing file. |
/// | `"a+b"` |	Open for reading and appending in binary mode. Creates a new file if it doesn't exist. |
///
/// # Example
///
/// ```
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/folder/file", "r", 1);
/// assert(f);
/// assert(0 == anylibs_fs_file_close(&f));
///
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/folder", "r", 1); // this will fail as this is a directory not a file
/// assert(!f);
/// assert(0 == anylibs_fs_file_close(&f)); // you can close the file even if it is invalid
/// ```
anylibs_fs_file_t* anylibs_fs_file_open(anylibs_fs_path_t path, char const mode[], size_t mode_size);

/// Return `0` if `anylibs_fs_file_t` is still open or `1` if not, this will basically check if `self` is `NULL` or not
///
/// # Example
///
/// ```
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/path/to/invalid", "r", 1);
/// assert(f);
/// int is_open = anylibs_fs_file_is_open(f);
/// assert(0 != is_open);
/// assert(0 == anylibs_fs_file_close(&f)); // you can close the file even if it is invalid
///
/// f = anylibs_fs_file_open("/folder/file", "r", 1);
/// assert(f);
/// int is_open = anylibs_fs_file_is_open(f);
/// assert(0 == is_open);
/// assert(0 == anylibs_fs_file_close(&f)); // don't forget to close the opened files
/// ```
int anylibs_fs_file_is_open(anylibs_fs_file_t* self);

/// Get file size through `out_file_size`.
/// Return `0` on success, `-1` on failure
///
/// # Example
///
/// ```
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/folder/file", "r", 1);
/// assert(f);
/// size_t file_size;
/// assert(0 == anylibs_fs_file_size(f, &file_size));
/// assert(0 > file_size);
/// assert(0 == anylibs_fs_file_close(&f)); // don't forget to close the opened files
/// ```
int anylibs_fs_file_size(anylibs_fs_file_t* self, size_t* out_file_size);

/// Read some data from an opened `anylibs_fs_file_t`.
/// Return `0` means error, otherwise the number represent the number of bytes that has been read
/// \note This will not zero terminate the result.
///
/// # Example
///
/// ```
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/folder/file", "r", 1); // "Hello, World!"
/// assert(f);
/// enum { buf_size = 100 };
/// char buf[buf_size];
/// assert(0 < anylibs_fs_file_read(f, buf, strlen("Hello, World!"), sizeof(char)));
///
/// assert(0 == anylibs_fs_file_close(&f)); // don't forget to close the opened files
/// ```
size_t anylibs_fs_file_read(anylibs_fs_file_t* self, char buf[], size_t elements_count, size_t element_size);

/// Write some data to an opened `anylibs_fs_file_t`
/// Return `0` means error, otherwise the number represent the number of bytes that has been writen
///
/// # Example
///
/// ```
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/folder/file", "w", 1);
/// assert(f);
/// char buf[] = "Hello, World!";
/// assert(0 < anylibs_fs_file_write(f, buf, strlen("Hello, World!"), sizeof(char)));
/// size_t fsize;
/// assert(0 == anylibs_fs_file_size(f, &fsize));
/// assert(strlen("Hello, World!") == fsize);
///
/// assert(0 == anylibs_fs_file_close(&f)); // don't forget to close the opened files
/// ```
size_t anylibs_fs_file_write(anylibs_fs_file_t* self, char buf[], size_t elements_count, size_t element_size);

/// Flush out the output stream (usually after you used `anylibs_fs_file_write`), this will get automatically call on `anylibs_fs_file_close`.
/// Return `0` on success, `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/folder/file", "w", 1);
/// assert(f);
/// char buf[] = "Hello, World!";
/// assert(0 < anylibs_fs_file_write(f, buf, strlen("Hello, World!"), sizeof(char)));
/// assert(0 == anylibs_fs_file_flush(f)); // now "Hello, World!" has been flushed to the file
///
/// assert(0 == anylibs_fs_file_close(&f)); // don't forget to close the opened files
/// ```
int anylibs_fs_file_flush(anylibs_fs_file_t* self);

/// Close an opened `anylibs_fs_file_t`, this function will not fail if self is NULL, it will also do `*self = NULL`.
/// Return `0` on success, `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_file_t* f = anylibs_fs_file_open("/folder/file", "r", 1);
/// assert(f);
/// assert(0 == anylibs_fs_file_close(&f));
/// ```
int anylibs_fs_file_close(anylibs_fs_file_t** self);

/// Get the maximum size that current OS can hold.
size_t anylibs_fs_path_max_size(void);

/// Append `anylibs_fs_path_t` to `anylibs_fs_pathbuf_t`. Return `0` on success, `-1` on error.
///  - This will add separator based on the current OS.
///  - It will add a trailing zero.
///  - It will update `anylibs_fs_pathbuf_t` size.
///
/// # Example
///
/// ```
/// enum { buf_size = 1000 };
/// char buf[buf_size] = "/folder";
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
/// assert(0 == anylibs_fs_path_append(&pathbuf, anylibs_fs_path_create("file", strlen("file"))));
/// assert(0 == strncmp("/folder/file", pathbuf.data, pathbuf.size));
/// ```
int anylibs_fs_path_append(anylibs_fs_pathbuf_t* base_path, anylibs_fs_path_t path);

/// Convert `anylibs_fs_path_t` to absoulte through `anylibs_fs_pathbuf_t`, make sure `anylibs_fs_pathbuf_t` has enough capacity.
/// Return `0` on success, `-1` on error.
///  - It will add a trailing zero.
///  - It will update `anylibs_fs_pathbuf_t` size.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("file", strlen("file")); // absolute: `/folder/file`
/// enum { buf_size = 100 };
/// char buf[buf_size];
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, 0, buf_size);
///
/// assert(0 == anylibs_fs_path_to_absolute(path, &pathbuf));
/// assert(0 == strncmp("/folder/file", pathbuf.data, pathbuf.size));
/// ```
int anylibs_fs_path_to_absolute(anylibs_fs_path_t path, anylibs_fs_pathbuf_t* in_out_abs_path);

/// Check if `anylibs_fs_path_t` is absolute, Return `0` if absolute, `1` if not, no error here.
/// \note This will not check if `anylibs_fs_path_t` exists or not.
///
/// # Example
///
/// ```
/// assert(0 == anylibs_fs_path_is_absolute(anylibs_fs_path_create("/absolute/path", strlen("/absolute/path"))));
/// assert(0 == anylibs_fs_path_is_absolute(anylibs_fs_path_create("/path/to/non-existing/file", strlen("/path/to/non-existing/file"))));
/// assert(1 == anylibs_fs_path_is_absolute(anylibs_fs_path_create(".", strlen("."))));
///
/// ```
int anylibs_fs_path_is_absolute(anylibs_fs_path_t path);

/// Update `anylibs_fs_path_t` to point to the parent, Return `0` on success, `-1` on error.
/// \note this will only update `anylibs_fs_path_t` `size`, neither the `data` pointer will not get updated nor there will be a trailing zero.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = CPATH("/folder/file");
/// assert(0 == anylibs_fs_path_parent(&path));
/// assert("/folder", path.data, strlen("/folder"));
///
/// path = CPATH("/");
/// assert(0 == anylibs_fs_path_parent(&path));
/// assert(0 == path.size);
/// assert(0 != anylibs_fs_path_parent(&path));
///
/// path = CPATH("folder");
/// assert(1, anylibs_fs_path_parent(&path));
///
/// ```
int anylibs_fs_path_parent(anylibs_fs_path_t* path);

/// Return the separator that is supported by the current OS
///
/// # Example
///
/// ```
/// #ifdef _WIN32
/// assert( '\\' == anylibs_fs_path_separator);
/// #else
/// assert( '/' == anylibs_fs_path_separator);
/// #endif
/// ```
char anylibs_fs_path_separator(void);

/// Update `anylibs_fs_path_t` to point to the filename, Return `0` on success, `1` on failure and `-1` on error.
/// \note this will only update `anylibs_fs_path_t` `size`, neither the `data` pointer will not get updated nor there will be a trailing zero.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder/file.txt", strlen("/folder/file.txt"));
/// assert(0 == anylibs_fs_path_filename(&path));
/// assert(0 == strncmp("file.txt", path.data, path.size));
///
/// path = anylibs_fs_path_create("/", 1);
/// assert(1 == c_Fs_path_filename(&path));
/// ```
int anylibs_fs_path_filename(anylibs_fs_path_t* path);

/// Get filestem (filename without the extension) of `anylibs_fs_path_t`, Return `0` on success, `1` on failure and `-1` on error.
/// \note this will only update `anylibs_fs_path_t` `size`, neither the `data` pointer will not get updated nor there will be a trailing zero.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder/file.txt", strlen("/folder/file.txt"));
/// assert(0 == anylibs_fs_path_filestem(&path));
/// assert(0 == strncmp("file", path.data, path.size));
///
/// path = anylibs_fs_path_create("/", 1);
/// assert(1 == c_Fs_path_filename(&path));
/// ```
int anylibs_fs_path_filestem(anylibs_fs_path_t* path);

/// Get the extension of `anylibs_fs_path_t`, Return `0` on success, `1` on failure and `-1` on error.
/// \note this will only update `anylibs_fs_path_t` `size`, neither the `data` pointer will not get updated nor there will be a trailing zero.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder/file.txt", strlen("/folder/file.txt"));
/// assert(0 == anylibs_fs_path_extension(&path));
/// assert(0 == strncmp("txt", path.data, path.size));
///
/// path = anylibs_fs_path_create("/", 1);
/// assert(1 == c_Fs_path_filename(&path));
/// ```
int anylibs_fs_path_file_extension(anylibs_fs_path_t* path);

/// Get `anylibs_fs_metadata_t` of `anylibs_fs_path_t`, Return `0` on success, and `-1` on error.
///
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder/file", strlen("/folder/file")); // a file containes "Hello, World!"
/// anylibs_fs_metadata_t metadata;
/// assert(0 == anylibs_fs_path_metadata(path, &metadata));
/// assert(strlen("Hello, World!") == metadata.fsize);
/// assert(C_FS_FILE_TYPE_file == metadata.ftype);
/// assert(C_FS_FILE_PERMISSION_read_write == metadata.fperm);
/// ```
int anylibs_fs_path_metadata(anylibs_fs_path_t path, anylibs_fs_metadata_t* out_metadata);

/// Create new directory at `anylibs_fs_path_t`, Return `0` on success, and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t dir_path = anylibs_fs_path_create("/folder");
/// assert(0 == anylibs_fs_dir_create(dir_path));
/// assert(0 == anylibs_fs_is_dir(dir_path));
/// ```
int anylibs_fs_dir_create(anylibs_fs_path_t dir_path);

/// Check whether this `anylibs_fs_path_t` exists and is a directory, Return `0` on success, `1` on failure and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t dir_path = anylibs_fs_path_create("/folder");
/// assert(0 == anylibs_fs_is_dir(dir_path));
///
/// dir_path = anylibs_fs_path_create("/folder/file");
/// assert(0 != anylibs_fs_is_dir(dir_path));
/// ```
int anylibs_fs_is_dir(anylibs_fs_path_t path);

/// Check whether this `anylibs_fs_path_t` exists and is a file, Return `0` on success, `1` on failure and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t file_path = anylibs_fs_path_create("/folder/file");
/// assert(0 == anylibs_fs_is_file(file_path));
///
/// file_path = anylibs_fs_path_create("/folder/none_exists_file");
/// assert(0 != anylibs_fs_is_file(file_path));
/// ```
int anylibs_fs_is_file(anylibs_fs_path_t path);

/// Check whether this `anylibs_fs_path_t` exists and is a symlink, Return `0` on success, `1` on failure and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t symlink_path = anylibs_fs_path_create("/folder/symlink");
/// assert(0 == anylibs_fs_is_symlink(symlink_path));
///
/// symlink_path = anylibs_fs_path_create("/folder/none_exists_symlink");
/// assert(0 != anylibs_fs_is_symlink(symlink_path));
/// ```
int anylibs_fs_is_symlink(anylibs_fs_path_t path);

/// Get current directory through `anylibs_fs_pathbuf_t`, Return `0` on success, and `-1` on error.
/// This will update `anylibs_fs_pathbuf_t` size and will append a trailing zero at the end.
///
/// # Example
///
/// ```
/// enum { pathbuf_size = 100 };
/// char pathbuf_buf[pathbuf_size];
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(pathbuf_buf, 0, pathbuf_size);
/// assert( 0 == anylibs_fs_dir_current(&pathbuf)); // current directory is `/folder`
/// assert( 0 == strncmp("/folder", pathbuf.data, strlen("/folder")) );
/// ```
int anylibs_fs_dir_current(anylibs_fs_pathbuf_t* in_out_dir_path);

/// Get executable path through `anylibs_fs_pathbuf_t`, Return `0` on success, and `-1` on error.
/// This will update `anylibs_fs_pathbuf_t` size and will append a trailing zero at the end.
///
/// # Example
///
/// ```
/// enum { pathbuf_size = 100 };
/// char pathbuf_buf[pathbuf_size];
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(pathbuf_buf, 0, pathbuf_size);
/// assert( 0 == anylibs_fs_dir_current_exe(&pathbuf)); // current executable path is `/folder/fs.exe`
/// assert( 0 == strncmp("/folder/fs.exe", pathbuf.data, strlen("/folder/fs.exe")) );
/// ```
int anylibs_fs_dir_current_exe(anylibs_fs_pathbuf_t* in_out_dir_path);

/// Change current directory, same like `cd` command on linux, Return `0` on success, and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder/some_other_directory", strlen("/folder/some_other_directory"));
/// assert( 0 == anylibs_fs_dir_change_current(path));
///
/// enum { pathbuf_size = 100 };
/// char pathbuf_buf[pathbuf_size];
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(pathbuf_buf, 0, pathbuf_size);
/// assert( 0 == anylibs_fs_dir_current(&pathbuf));
/// assert( 0 == strncmp("/folder/some_other_directory", pathbuf.data, strlen("/folder/some_other_directory")) );
/// ```
int anylibs_fs_dir_change_current(anylibs_fs_path_t new_path);

/// Check if `anylibs_fs_path_t` as a directory is empty, Return `0` on success, `1` on failure and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t dir_path = anylibs_fs_path_create("/folder", str("/folder")); // this folder has `file.txt`
/// assert(1 == anylibs_fs_dir_is_empty(dir_path));
///
/// dir_path = anylibs_fs_path_create("/folder2", str("/folder2")); // this folder is empty
/// assert(0 == anylibs_fs_dir_is_empty(dir_path));
/// ```
int anylibs_fs_dir_is_empty(anylibs_fs_path_t path);

/// Check whether this `anylibs_fs_path_t` exists or not, Return `0` on success, `1` on failure and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder", strlen("/folder")); // this folder exists
/// assert(0 == anylibs_fs_exists(path));
///
/// path = anylibs_fs_path_create("/none/existing/folder", strlen("/none/existing/folder"));
/// assert(0 != anylibs_fs_exists(path));
/// ```
int anylibs_fs_exists(anylibs_fs_path_t path);

/// Delete a file or an empty directory, Return `0` on success, and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder/file", strlen("/folder/file"));
/// assert(0 == anylibs_fs_delete(path));
/// assert(1 == anylibs_fs_path_exists(path));
/// ```
int anylibs_fs_delete(anylibs_fs_path_t path);

/// Delete empty/non-empty directory, Return `0` on success, and `-1` on error.
///
/// # Example
///
/// ```
/// enum { buf_size = 100 };
/// buf[buf_size] = "/folder"; //none empty directory
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen("/folder"), buf_size);
/// assert(0 == anylibs_fs_delete_recursively(pathbuf));
/// assert(1 == anylibs_fs_path_exists(anylibs_fs_path_create(pathbuf.data, pathbuf.size)));
/// ```
int anylibs_fs_delete_recursively(anylibs_fs_pathbuf_t* path);

/// Create an `anylibs_fs_iter_t` to iterate on files/directories inside `anylibs_fs_pathbuf_t`, Return `0` on success, and `-1` on error.
/// `anylibs_fs_pathbuf_t` will get updated on each iteration (size, data, capacity).
///
/// # Example
///
/// ```
/// enum { buf_size = 100 };
/// buf[buf_size] = "/folder"; // this has `file1`, `file2`, `folder1`
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
///
/// anylibs_fs_iter_t iter;
/// int status;
/// anylibs_fs_pathref_t ref;
/// assert(0 == anylibs_fs_iter(&pathbuf, &iter));
/// while((status = anylibs_fs_iter_next(&iter, &ref)) == 0)
/// {
///    puts(ref.data); `file1`, `file2`, `folder1`
/// }
/// assert(1 == status);
/// assert(0 == anylibs_fs_iter_close(&iter));
/// ```
int anylibs_fs_iter_create(anylibs_fs_pathbuf_t* path, anylibs_fs_iter_t* out_iter);

/// Advance the `anylibs_fs_iter_t` to the next file/directory.
/// Return `0` on success, `1` on no more files/directories and `-1` on error.
///
/// # Example
///
/// ```
/// enum { buf_size = 100 };
/// buf[buf_size] = "/folder"; // this has `file1`, `file2`, `folder1`
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
///
/// anylibs_fs_iter_t iter;
/// int status;
/// anylibs_fs_pathref_t ref;
/// assert(0 == anylibs_fs_iter(&pathbuf, &iter));
/// while((status = anylibs_fs_iter_next(&iter, &ref)) == 0)
/// {
///    puts(ref.data); `file1`, `file2`, `folder1`
/// }
/// assert(1 == status);
/// assert(0 == anylibs_fs_iter_close(&iter));
/// ```
int anylibs_fs_iter_next(anylibs_fs_iter_t* iter, anylibs_fs_pathref_t* out_cur_path);

/// Close an `anylibs_fs_iter_t` to the next file/directory, Return `0` on success, and `-1` on error.
/// This will restore `anylibs_fs_pathbuf_t` to its original state before iteration.
///
/// # Example
///
/// ```
/// enum { buf_size = 100 };
/// buf[buf_size] = "/folder"; // this has `file1`, `file2`, `folder1`
/// anylibs_fs_pathbuf_t pathbuf = anylibs_fs_pathbuf_create(buf, strlen(buf), buf_size);
///
/// anylibs_fs_iter_t iter;
/// int status;
/// anylibs_fs_pathref_t ref;
/// assert(0 == anylibs_fs_iter(&pathbuf, &iter));
/// while((status = anylibs_fs_iter_next(&iter, &ref)) == 0)
/// {
///    puts(ref.data); `file1`, `file2`, `folder1`
/// }
/// assert(1 == status);
/// assert(0 == anylibs_fs_iter_close(&iter));
/// ```
int anylibs_fs_iter_destroy(anylibs_fs_iter_t* iter);

/// Get last component of a `anylibs_fs_path_t` and return it through `out_last_component`, Return `0` on success, and `-1` on error.
///
/// # Example
///
/// ```
/// anylibs_fs_path_t path = anylibs_fs_path_create("/folder/file1", strlen("/folder/file1"));
/// anylibs_fs_path_t last_component;
///
/// assert(0 == anylibs_fs_path_get_last_component(path, &last_component));
/// assert(0 == strncmp("file1", last_component.data, last_component.size));
/// ```
int anylibs_fs_path_get_last_component(anylibs_fs_path_t path, anylibs_fs_path_t* out_last_component);

/// Convert `anylibs_fs_pathref_t` to `anylibs_fs_path_t`. This is a cheap conversion, no new memory for `data` created.
anylibs_fs_path_t anylibs_fs_pathref_to_path(anylibs_fs_pathref_t path_ref);

/// Convert `anylibs_fs_pathbuf_t` to `anylibs_fs_pathref_t`. This is a cheap conversion, no new memory for `data` created.
anylibs_fs_pathref_t anylibs_fs_pathbuf_to_pathref(anylibs_fs_pathbuf_t pathbuf);

/// Convert `anylibs_fs_pathbuf_t` to `anylibs_fs_path_t`. This is a cheap conversion, no new memory for `data` created.
anylibs_fs_path_t anylibs_fs_cpathbuf_to_cpath(anylibs_fs_pathbuf_t pathbuf);

#endif // ANYLIBS_FS_H
