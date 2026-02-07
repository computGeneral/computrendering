
/*****************************************************************************
Singleton configuration class for d3dplayer
*****************************************************************************/

#ifndef __D3DCONFIGURATION
#define __D3DCONFIGURATION

class D3DTraceConfig {
    public:
        void load(std::string filename);
        std::string getValue(std::string section, std::string variable);
        bool existsVariable(std::string section, std::string variable);

        static D3DTraceConfig &instance();
    private:
        IniFile ini;
};

#endif
