<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\I18n\DMsg;

class ConfIconTitleResolver
{
    public static function resolve($view, $viewName = null)
    {
        switch ($view) {
            case 'serv':
                return ['server-cog', DMsg::UIStr('menu_serv')];
            case 'sl':
                return ['plug-zap', DMsg::UIStr('menu_sl')];
            case 'sl_':
                return ['plug-zap', DMsg::UIStr('menu_sl_') . ' ' . $viewName];
            case 'vh':
                return ['server', DMsg::UIStr('menu_vh')];
            case 'vh_':
                return ['server', DMsg::UIStr('menu_vh_') . ' ' . $viewName];
            case 'tp':
                return ['copy', DMsg::UIStr('menu_tp')];
            case 'tp_':
                return ['copy', DMsg::UIStr('menu_tp_') . ' ' . $viewName];
            case 'lb':
                return ['network', DMsg::UIStr('menu_lb')];
            case 'lb_':
            case 'lb4_':
                return ['network', DMsg::UIStr('menu_lb_') . ' ' . $viewName];
            case 'ha':
                return ['shield-check', DMsg::UIStr('menu_ha')];
            case 'admin':
                return ['settings', DMsg::UIStr('menu_webadmin')];
            case 'al':
                return ['plug-zap', DMsg::UIStr('menu_webadmin') . ' - ' . DMsg::UIStr('menu_sl')];
            case 'al_':
                return ['plug-zap', DMsg::UIStr('menu_webadmin') . ' - ' . DMsg::UIStr('menu_sl_') . ' ' . $viewName];
        }

        return [null, null];
    }
}
