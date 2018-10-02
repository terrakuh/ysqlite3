#pragma once

#define SQLITE3_ENCRYPTED_VFS_NAME "encrypted-vfs"


namespace sqlite3
{

void register_encrypted_vfs();
void unregister_encrypted_vfs();

}