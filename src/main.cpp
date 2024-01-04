#include "utils/logger.h"
#include <coreinit/filesystem.h>
#include <malloc.h>
#include <string.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("webMAN Cafe");
WUPS_PLUGIN_DESCRIPTION("Aroma Web Control Plugin");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("ruigegoat");
WUPS_PLUGIN_LICENSE("")

#define LOG_FS_OPEN_CONFIG_ID "logFSOpen"

/**
    All of this defines can be used in ANY file.
    It's possible to split it up into multiple files.

**/

WUPS_USE_WUT_DEVOPTAB();            // Use the wut devoptabs
WUPS_USE_STORAGE("wm_cafe"); // Unique id for the storage api

bool logFSOpen = true;

/**
    Gets called ONCE when the plugin was loaded.
**/
INITIALIZE_PLUGIN() {
    // Logging only works when compiled with `make DEBUG=1`. See the README for more information.
    initLogging();
    DEBUG_FUNCTION_LINE("webMAN Cafe initialise baslangici");

    // Open storage to read values
    WUPSStorageError storageRes = WUPS_OpenStorage();
    if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Depolama alani acilamadi %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
    } else {
        // Try to get value from storage
        if ((storageRes = WUPS_GetBool(nullptr, LOG_FS_OPEN_CONFIG_ID, &logFSOpen)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it's missing.
            if (WUPS_StoreBool(nullptr, LOG_FS_OPEN_CONFIG_ID, logFSOpen) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE("Bool değeri koyulamadi");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Bool değeri alinamadi %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        // Close storage
        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Depolama alani kapatilamadi");
        }
    }
    deinitLogging();
}

/**
    Gets called when the plugin will be unloaded.
**/
DEINITIALIZE_PLUGIN() {
    DEBUG_FUNCTION_LINE("webMAN Cafe deinit");
}

/**
    Gets called when an application starts.
**/
ON_APPLICATION_START() {
    initLogging();

    DEBUG_FUNCTION_LINE("WebMan-Cafe uygulama acildi, gerekli güncellemeyi yap");
}

/**
 * Gets called when an application actually ends
 */
ON_APPLICATION_ENDS() {
    deinitLogging();
}

/**
    Gets called when an application request to exit.
**/
ON_APPLICATION_REQUESTS_EXIT() {
    DEBUG_FUNCTION_LINE_INFO("WebMan-Cafe uygulama kapanmak istiyor, gerekli guncellemeyi yap");
}

/**
 * Callback that will be called if the config has been changed
 */
void logFSOpenChanged(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE_INFO("New value in logFSOpenChanged: %d", newValue);
    logFSOpen = newValue;
    // If the value has changed, we store it in the storage.
    WUPS_StoreInt(nullptr, LOG_FS_OPEN_CONFIG_ID, logFSOpen);
}

WUPS_GET_CONFIG() {
    // We open the storage, so we can persist the configuration the user did.
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Example Plugin");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Logging", &cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, LOG_FS_OPEN_CONFIG_ID, "Log FSOpen calls", logFSOpen, &logFSOpenChanged);

    return config;
}

WUPS_CONFIG_CLOSED() {
    // Save all changes
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to close storage");
    }
}

/**
    This defines a function replacement.
    It allows to replace the system function with an own function. 
    So whenever a game / application calls an overridden function, your function gets called instead.
    
    Currently it's only possible to override functions that are loaded from .rpl files of OSv10 (00050010-1000400A).
    
    Signature of this macro:
    DECL_FUNCTION(  RETURN_TYPE, ARBITRARY_NAME_OF_FUNCTION , ARGS_SEPERATED_BY_COMMA){
        //Your code goes here.
    }
    
    Within this macro, two more function get declare you can use.
    my_ARBITRARY_NAME_OF_FUNCTION and real_ARBITRARY_NAME_OF_FUNCTION
    
    RETURN_TYPE my_ARBITRARY_NAME_OF_FUNCTION(ARGS_SEPERATED_BY_COMMA):
        is just name of the function that gets declared in this macro. 
        It has the same effect as calling the overridden function directly.
    
    RETURN_TYPE real_ARBITRARY_NAME_OF_FUNCTION(ARGS_SEPERATED_BY_COMMA):
        is the name of the function, that leads to function that was overridden.
        Use this to call the original function that will be overridden.
        CAUTION: Other plugins may already have manipulated the return value or arguments.

    Use this macro for each function you want to override
**/
DECL_FUNCTION(int, FSOpenFile, FSClient *pClient, FSCmdBlock *pCmd, const char *path, const char *mode, int *handle, int error) {
    int result = real_FSOpenFile(pClient, pCmd, path, mode, handle, error);
    if (logFSOpen) {
        DEBUG_FUNCTION_LINE_INFO("FSOpenFile called for folder %s! Result %d", path, result);
    }
    return result;
}

/**
This tells the loader which functions from which library (.rpl) should be replaced with which function from this file.
The list of possible libraries can be found in the wiki. (In general it's WUPS_LOADER_LIBRARY_ + the name of the RPL in caps lock)

WUPS_MUST_REPLACE(FUNCTION_NAME_IN_THIS_FILE,   NAME_OF_LIB_WHICH_CONTAINS_THIS_FUNCTION,   NAME_OF_FUNCTION_TO_OVERRIDE)

Define this for each function you want to override.
**/
WUPS_MUST_REPLACE(FSOpenFile, WUPS_LOADER_LIBRARY_COREINIT, FSOpenFile);
