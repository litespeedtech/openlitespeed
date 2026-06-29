<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\UI\ConfPageResolver;

abstract class DPageDefBase
{
    private static $instances = [];

    protected $_pageDef = [];
    protected $_fileDef = [];

    protected function __construct()
    {
        $this->defineAll();
    }

    public static function GetInstance()
    {
        $class = get_called_class();
        if (!isset(self::$instances[$class])) {
            self::$instances[$class] = new $class();
        }
        return self::$instances[$class];
    }

    public static function GetPage($dinfo)
    {
        return ConfPageResolver::resolve($dinfo, get_called_class());
    }

    public function ResolvePage($view, $pid)
    {
        return $this->_pageDef[$view][$pid];
    }

    public function GetFileMap($type)
    {
        if (!isset($this->_fileDef[$type])) {
            $funcname = 'add_FileMap_' . $type;
            if (!method_exists($this, $funcname)) {
                trigger_error("invalid func name $funcname", E_USER_ERROR);
            }
            $this->$funcname();
        }
        return $this->_fileDef[$type];
    }

    protected function add_FileMap_admin()
    {
        $map = new DTblMap(['adminConfig', ''],
            [
                'ADM_PHP',
                'ADM_THROTTLE',
                new DTblMap(['logging:log', 'errorlog$fileName'], 'V_LOG'),
                new DTblMap(['logging:accessLog', 'accesslog$fileName'], 'ADM_ACLOG'),
                new DTblMap(['security:accessControl', 'accessControl'], 'ADM_SEC_AC'),
                new DTblMap(['listenerList:*listener', '*listener$name'],
                    ['ADM_L_GENERAL', 'ALVT_SSL_CERT', 'AL_SSL', 'AL_SSL_FEATURE', 'ALVT_SSL_CLVERIFY'])
            ]);

        $this->_fileDef['admin'] = $map;
    }

    public function GetTabDef($view)
    {
        if (!isset($this->_pageDef[$view])) {
            trigger_error("Invalid tabs $view", E_USER_ERROR);
        }

        $tabs = [];
        foreach ($this->_pageDef[$view] as $p) {
            $tabs[$p->GetID()] = $p->GetLabel();
        }
        return $tabs;
    }

    abstract protected function defineAll();
}
