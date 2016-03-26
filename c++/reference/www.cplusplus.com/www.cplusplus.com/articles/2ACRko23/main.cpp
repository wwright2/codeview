#include <iostream>

/// http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx#macros_for_conditional_declarations
#define _WIN32_WINNT 0x05010300 //Windows XP SP3

#include <windows.h>
#include <winbase.h>
#include <aclapi.h>
#include <sddl.h>


void pause()
{
    std::cin.sync();
    std::cin.ignore();
}


int main()
{
    /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa374931(v=vs.85).aspx
    PACL pOldDacl = new ACL;
    PACL pNewDacl = new ACL;

    HANDLE hFile = NULL;

    PSECURITY_DESCRIPTOR pSecurityDescriptor = new SECURITY_DESCRIPTOR;
        /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa378863(v=vs.85).aspx
        InitializeSecurityDescriptor(pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

    char FileName[MAX_PATH];
    char CurrentDirectory[256];
    char Error[256];

    /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa364934(v=vs.85).aspx
    GetCurrentDirectory(256, CurrentDirectory);

    /// http://msdn.microsoft.com/en-us/library/ce3zzk1k.aspx
    sprintf_s(FileName, MAX_PATH, "%s\\New.txt", CurrentDirectory);

    LPSTR* DACLDescriptorAsString = new LPSTR;
    //LPSTR* SACLDescriptorAsString = new LPSTR;

    DWORD ValueReturned = 0;
    DWORD SecDescStringLengthNeeded = 0;

    const ULONG NumberOfExplicitAccessEntries = 3;

    /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa446627(v=vs.85).aspx
    EXPLICIT_ACCESS ExplicitAccess[NumberOfExplicitAccessEntries];
        ZeroMemory(&ExplicitAccess[0], sizeof(EXPLICIT_ACCESS));
        /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa376378(v=vs.85).aspx
        BuildExplicitAccessWithName(&ExplicitAccess[0], "Guest", ACCESS_SYSTEM_SECURITY | WRITE_OWNER | WRITE_DAC | DELETE | GENERIC_WRITE, DENY_ACCESS, NO_PROPAGATE_INHERIT_ACE);

        /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa366920(v=vs.85).aspx
        ZeroMemory(&ExplicitAccess[1], sizeof(EXPLICIT_ACCESS));
            ExplicitAccess[1].grfAccessPermissions = GENERIC_ALL;
            ExplicitAccess[1].grfAccessMode        = GRANT_ACCESS;
            ExplicitAccess[1].grfInheritance       = NO_INHERITANCE;
                /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379636(v=vs.85).aspx
                ExplicitAccess[1].Trustee.pMultipleTrustee          = NULL;
                ExplicitAccess[1].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
                ExplicitAccess[1].Trustee.TrusteeForm               = TRUSTEE_IS_NAME;  /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379638(v=vs.85).aspx
                ExplicitAccess[1].Trustee.TrusteeType               = TRUSTEE_IS_GROUP; /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379639(v=vs.85).aspx
                ExplicitAccess[1].Trustee.ptstrName                 = "Authenticated Users";

        ZeroMemory(&ExplicitAccess[2], sizeof(EXPLICIT_ACCESS));
            ExplicitAccess[2].grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE | READ_CONTROL;
            ExplicitAccess[2].grfAccessMode        = GRANT_ACCESS;
            ExplicitAccess[2].grfInheritance       = NO_INHERITANCE;
                ExplicitAccess[2].Trustee.pMultipleTrustee          = NULL;
                ExplicitAccess[2].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
                ExplicitAccess[2].Trustee.TrusteeForm               = TRUSTEE_IS_NAME;
                ExplicitAccess[2].Trustee.TrusteeType               = TRUSTEE_IS_USER;
                ExplicitAccess[2].Trustee.ptstrName                 = "Guest";

    try
    {
        /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa363915(v=vs.85).aspx
        DeleteFile(FileName);

        /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858(v=vs.85).aspx
        hFile = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hFile == INVALID_HANDLE_VALUE)
        {
            sprintf_s(Error, "COULD NOT OPEN FILE: %s\n Error Code: ", FileName);
            throw Error;
        }
                         /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa446645(v=vs.85).aspx
        ValueReturned = GetNamedSecurityInfo(FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDacl, NULL, &pSecurityDescriptor);

        if(ValueReturned != ERROR_SUCCESS)
        {
            sprintf_s(Error, "Error: %d\nFor:%s\n", ValueReturned, FileName);
            throw Error;
        }

            /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa376397(v=vs.85).aspx
        if(!ConvertSecurityDescriptorToStringSecurityDescriptor(pSecurityDescriptor, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, DACLDescriptorAsString, &SecDescStringLengthNeeded))
        {
            throw "FAILED CONVERTING SECURITY DESCRIPTOR TO STRING: ";
        }

        /// SECURITY DESCRIPTOR STRING FORMAT: http://msdn.microsoft.com/en-us/library/windows/desktop/aa379570(v=vs.85).aspx
        /// ACE STRING FORMAT: http://msdn.microsoft.com/en-us/library/windows/desktop/aa374928(v=vs.85).aspx
        /// SID STRING FORMAT: http://msdn.microsoft.com/en-us/library/windows/desktop/aa379602(v=vs.85).aspx
        std::cout << "Current Discretionary Security Descriptor As A String: " << *DACLDescriptorAsString << "\n\n";

        std::cout << "NOW YOU SHOULD CHECK THE SECURITY PERMISSIONS ON THE TARGET FILE\n\n";
        pause();

                   /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379576(v=vs.85).aspx
        ValueReturned = SetEntriesInAcl(NumberOfExplicitAccessEntries, ExplicitAccess, pOldDacl, &pNewDacl);

        if(ValueReturned != ERROR_SUCCESS)
        {
            sprintf_s(Error, "Error: %d WHILE SETTING NEW ACLs\n", ValueReturned);
            throw Error;
        }

            /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379142(v=vs.85).aspx
        if(!IsValidAcl(pNewDacl))
        {
            throw "Invalid ACL: ";
        }

                   /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa379579(v=vs.85).aspx
        ValueReturned = SetNamedSecurityInfo(FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL);

        if(ValueReturned != ERROR_SUCCESS)
        {
            sprintf_s(Error, "Error: %d WHILE SETTING NAMED SECURITY INFO\n", ValueReturned);
            throw Error;
        }

        ValueReturned = GetNamedSecurityInfo(FileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDacl, NULL, &pSecurityDescriptor);

        if(ValueReturned != ERROR_SUCCESS)
        {
            sprintf_s(Error, "Error: %d\nFor:%s\n", ValueReturned, FileName);
            throw Error;
        }

            /// http://msdn.microsoft.com/en-us/library/windows/desktop/aa376397(v=vs.85).aspx
        if(!ConvertSecurityDescriptorToStringSecurityDescriptor(pSecurityDescriptor, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, DACLDescriptorAsString, &SecDescStringLengthNeeded))
        {
            throw "FAILED CONVERTING SECURITY DESCRIPTOR TO STRING: ";
        }

        std::cout << "New Security Descriptor As A String: " << *DACLDescriptorAsString << "\n\n";

        MessageBox(NULL, "Security Permissions Have Been Changed On Target File", "Security Permissions Changed", MB_OK | MB_DEFBUTTON1);

    }
    catch(char* ErrorMessage)
    {
        std::cout << ErrorMessage << GetLastError() << std::endl;
    }

    CloseHandle(hFile);

    delete pOldDacl;
    delete pNewDacl;
    delete DACLDescriptorAsString;
    //delete SACLDescriptorAsString;

    if(LocalFree(pNewDacl) != NULL)
    {
        std::cout << "Error Freeing DACL: " << GetLastError() << std::endl;
    }

    if(LocalFree(pSecurityDescriptor) != NULL)
    {
        std::cout << "Error Freeing Security Descriptor: " << GetLastError() << std::endl;
    }
    delete pSecurityDescriptor;

    return 0;
}
