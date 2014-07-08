
#include <../addon/include/ls.h>

#include <string.h>

extern lsi_module_t modcompress;
extern lsi_module_t moddecompress;
extern int addModgzipFilter(lsi_session_t *session, int isSend, uint8_t compressLevel, int priority);
typedef struct 
{
    const char      * _pName;
    lsi_module_t    * _pModule;
} Prelinked_Module;

Prelinked_Module g_prelinked[] = 
{
    { "modcompress", &modcompress   },
    { "moddecompress", &moddecompress   },
};

int getPrelinkedModuleCount()
{
    return sizeof( g_prelinked ) / sizeof( Prelinked_Module );
}

lsi_module_t * getPrelinkedModuleByIndex( unsigned int index, const char ** pName )
{
    if ( index >= sizeof( g_prelinked ) / sizeof( Prelinked_Module ) )
        return NULL;
    *pName = g_prelinked[index]._pName;
    return g_prelinked[index]._pModule;
}

// lsi_module_t * getPrelinkedModule( const char * pModule )
// {
//     for( int i=0; i < sizeof( g_prelinked ) / sizeof( Prelinked_Module );++i )
//     {
//         if ( strcmp( g_prelinked[i]._pName, pModule ) == 0 )
//             return g_prelinked[i]._pModule;
//     }
//     return NULL;
// }




