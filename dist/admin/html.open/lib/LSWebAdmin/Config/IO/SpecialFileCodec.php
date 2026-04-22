<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\Config\CNode;

class SpecialFileCodec
{
    public static function load($path, $id)
    {
        $lines = file($path);
        if ($lines === false) {
            return false;
        }

        $root = new CNode(CNode::K_ROOT, $id, CNode::T_ROOT);
        $items = [];

        if ($id == 'MIME') {
            foreach ($lines as $line) {
                if (($c = strpos($line, '=')) > 0) {
                    $suffix = trim(substr($line, 0, $c));
                    $type = trim(substr($line, $c + 1));
                    $m = new CNode('index', $suffix);
                    $m->AddChild(new CNode('suffix', $suffix));
                    $m->AddChild(new CNode('type', $type));
                    $items[$suffix] = $m;
                }
            }
        } elseif ($id == 'ADMUSR' || $id == 'V_UDB') {
            foreach ($lines as $line) {
                $parsed = explode(':', trim($line));
                $size = count($parsed);
                if ($size == 2 || $size == 3) {
                    $name = trim($parsed[0]);
                    $pass = trim($parsed[1]);
                    if ($name != '' && $pass != '') {
                        $u = new CNode('index', $name);
                        $u->AddChild(new CNode('name', $name));
                        $u->AddChild(new CNode('passwd', $pass));
                        if ($size == 3 && (($group = trim($parsed[2])) != '')) {
                            $u->AddChild(new CNode('group', $group));
                        }
                        $items[$name] = $u;
                    }
                }
            }
        } elseif ($id == 'V_GDB') {
            foreach ($lines as $line) {
                $parsed = explode(':', trim($line));
                if (count($parsed) == 2) {
                    $group = trim($parsed[0]);
                    $users = trim($parsed[1]);
                    if ($group != '') {
                        $g = new CNode('index', $group);
                        $g->AddChild(new CNode('name', $group));
                        $g->AddChild(new CNode('users', $users));
                        $items[$group] = $g;
                    }
                }
            }
        }

        ksort($items, SORT_STRING);
        reset($items);
        foreach ($items as $item) {
            $root->AddChild($item);
        }

        return $root;
    }

    public static function save($path, $id, $root)
    {
        $fd = fopen($path, 'w');
        if (!$fd) {
            return false;
        }

        $items = $root->GetChildren('index');

        if ($items != null) {
            if (is_array($items)) {
                ksort($items, SORT_STRING);
                reset($items);
            } else {
                $items = [$items];
            }

            foreach ($items as $key => $item) {
                $line = self::formatLine($id, $key, $item);
                if ($line !== '') {
                    fputs($fd, $line);
                }
            }
        }
        fclose($fd);
        return true;
    }

    private static function formatLine($id, $key, $item)
    {
        if ($id == 'MIME') {
            return str_pad($key, 8) . ' = ' . $item->GetChildVal('type') . "\n";
        }

        if ($id == 'ADMUSR' || $id == 'V_UDB') {
            $line = $item->GetChildVal('name') . ':' . $item->GetChildVal('passwd');
            $group = $item->GetChildVal('group');
            if ($group) {
                $line .= ':' . $group;
            }
            return $line . "\n";
        }

        if ($id == 'V_GDB') {
            return $item->GetChildVal('name') . ':' . $item->GetChildVal('users') . "\n";
        }

        return '';
    }
}
