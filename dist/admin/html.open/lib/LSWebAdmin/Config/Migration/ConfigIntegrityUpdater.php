<?php

namespace LSWebAdmin\Config\Migration;

use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Config\CNode;

class ConfigIntegrityUpdater
{
    public static function update($tid, $newref, $disp)
    {
        $ref = $disp->GetLastRef();
        if ($ref == null || $newref == $ref) {
            return;
        }

        if (in_array($tid, ['ADM_L_GENERAL', 'T_TOPD', 'V_TOPD', 'V_BASE', 'L_GENERAL'])) {
            $disp->SetViewName($newref);
        }

        $root = $disp->GetConfData()->GetRootNode();
        $dispView = $disp->GetView();

        if ($tid == 'V_BASE' || $tid == 'V_TOPD') {
            self::updateListenerVhMapRefs($root, $ref, $newref);
        }

        if ($newref == null) {
            return;
        }

        if ($tid == 'L_GENERAL') {
            self::updateTemplateListenerRefs($root, $ref, $newref);
        } elseif (strncmp($tid, 'A_EXT_', 6) == 0) {
            self::updateExternalAppRefs($root, $dispView, $ref, $newref);
        } elseif (strpos($tid, '_REALM_') !== false) {
            self::updateRealmRefs($root, $dispView, $ref, $newref);
        }
    }

    private static function updateListenerVhMapRefs($root, $ref, $newref)
    {
        $listeners = self::normalizeChildren($root->GetChildren('listener'));
        foreach ($listeners as $listener) {
            $maps = self::normalizeChildren($listener->GetChildren('vhmap'));
            foreach ($maps as $map) {
                if ($map->Get(CNode::FLD_VAL) == $ref) {
                    if ($newref == null) {
                        $map->RemoveFromParent();
                    } else {
                        $map->SetVal($newref);
                        if ($map->GetChildren('vhost') != null) {
                            $map->SetChildVal('vhost', $newref);
                        }
                    }
                    break;
                }
            }
        }
    }

    private static function updateTemplateListenerRefs($root, $ref, $newref)
    {
        $templates = self::normalizeChildren($root->GetChildren('vhTemplate'));
        foreach ($templates as $template) {
            $listeners = $template->GetChildVal('listeners');
            if ($listeners == null) {
                continue;
            }

            $changed = false;
            $items = preg_split("/, /", $listeners, -1, PREG_SPLIT_NO_EMPTY);
            foreach ($items as $i => $listener) {
                if ($listener == $ref) {
                    $items[$i] = $newref;
                    $changed = true;
                    break;
                }
            }
            if ($changed) {
                $template->SetChildVal('listeners', implode(', ', $items));
            }
        }
    }

    private static function updateExternalAppRefs($root, $dispView, $ref, $newref)
    {
        $scriptHandlerLoc = ($dispView == DInfo::CT_TP) ? 'virtualHostConfig:scripthandler:addsuffix' : 'scripthandler:addsuffix';
        $handlers = self::normalizeChildren($root->GetChildren($scriptHandlerLoc));
        foreach ($handlers as $handler) {
            if ($handler->GetChildVal('handler') == $ref) {
                $handler->SetChildVal('handler', $newref);
            }
        }

        if ($dispView != DInfo::CT_SERV) {
            $contextLoc = ($dispView == DInfo::CT_TP) ? 'virtualHostConfig:context' : 'context';
            $contexts = self::normalizeChildren($root->GetChildren($contextLoc));
            foreach ($contexts as $context) {
                if ($context->GetChildVal('authorizer') == $ref) {
                    $context->SetChildVal('authorizer', $newref);
                }
                if ($context->GetChildVal('handler') == $ref) {
                    $context->SetChildVal('handler', $newref);
                }
            }
        }
    }

    private static function updateRealmRefs($root, $dispView, $ref, $newref)
    {
        $contextLoc = ($dispView == DInfo::CT_TP) ? 'virtualHostConfig:context' : 'context';
        $contexts = self::normalizeChildren($root->GetChildren($contextLoc));
        foreach ($contexts as $context) {
            if ($context->GetChildVal('realm') == $ref) {
                $context->SetChildVal('realm', $newref);
            }
        }
    }

    private static function normalizeChildren($nodes)
    {
        if ($nodes == null) {
            return [];
        }

        if (is_array($nodes)) {
            return $nodes;
        }

        return [$nodes];
    }
}
