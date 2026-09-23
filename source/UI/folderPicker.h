#ifndef NXMP_FOLDER_PICKER_H
#define NXMP_FOLDER_PICKER_H

#include <string>
#include <vector>

namespace Windows {

void FolderPickerWindow(bool *focus, bool *first_item);
void InitFolderPicker(const std::string &initialPath);

} // namespace Windows

#endif // NXMP_FOLDER_PICKER_H
