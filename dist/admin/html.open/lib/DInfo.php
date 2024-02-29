<?php

class DInfo
{

    const FLD_ConfType = 1;
    const FLD_ConfErr = 2;
    const FLD_View = 3;
    const FLD_ViewName = 4;
    const FLD_TopMsg = 5;
    const FLD_ICONTITLE = 6;
    const FLD_CtrlUrl = 10;
    const FLD_MID = 11;
    const FLD_PID = 12;
    const FLD_TID = 13;
    const FLD_REF = 14;
    const FLD_ACT = 15;
    const FLD_SORT = 16;
    const FLD_TOKEN = 17;
    const FLD_PgData = 18;
    const FLD_ConfData = 19;
    const FLD_ServData = 20;
    // conftype
    const CT_SERV = 'serv';
    const CT_VH = 'vh_';
    const CT_TP = 'tp_';
    const CT_ADMIN = 'admin';
    const CT_EX = 'special';

    private $_confType;
    private $_view; //_type (serv, sl, sl_, vh, vh_, tp, tp_, lb, lb_, admin, al, al_)
    private $_viewName = null; // _name
    private $_ctrlUrl = 'index.php#view/confMgr.php?';
    private $_mid = 'serv'; // default
    private $_pid = null;
    private $_tid = null;
    private $_ref = null;
    private $_act;
    private $_token;
    private $_confErr;
    private $_pageData;
    private $_confData;
    private $_servData; // for populate vh level derived options
    private $_tabs;
    private $_sort;
    private $_topMsg;
    private $_isPrintingLinkedTbl;
    private $_allActions;

    private static $_instance;

	public static function singleton()
	{
		if (!isset(self::$_instance)) {
			self::$_instance = new self();
		}

		return self::$_instance;
	}

    private function __construct()
    {

    }

    public function ShowDebugInfo()
    {
        return "DINFO: conftype={$this->_confType} view={$this->_view} viewname={$this->_viewName} mid={$this->_mid} pid={$this->_pid} tid={$this->_tid} ref={$this->_ref} act={$this->_act}\n";
    }

    public function InitConf()
    {
        $has_pid = false;
        $mid = UIBase::GrabGoodInput("get_post", 'm');

        if ($mid != null) {
            $this->_mid = $mid;
            $pid = UIBase::GrabGoodInput("get_post", 'p');
            if ($pid != null) {
                $this->_pid = $pid;
                $has_pid = true;
            }
        }

        if (($pos = strpos($this->_mid, '_')) > 0) {
			$this->_view = substr($this->_mid, 0, $pos + 1);
			$this->_viewName = substr($this->_mid, $pos + 1);
			if ($this->_pid == '' || $this->_view == 'sl' || $this->_view == 'sl_' || $this->_view == 'al' || $this->_view == 'al_' || $this->_pid == 'base' || $this->_pid == 'mbr') {
				$this->_ref = $this->_viewName; // still in serv conf
			}
		} else {
			$this->_view = $this->_mid;
		}

		$this->_confType = ( $this->_mid[0] == 'a' ) ? self::CT_ADMIN : self::CT_SERV;

        $this->_tabs = DPageDef::GetInstance()->GetTabDef($this->_view);

        if ($has_pid) {
            if (!array_key_exists($this->_pid, $this->_tabs))
                die("Invalid pid - {$this->_pid} \n");
        }
        else {
            $this->_pid = key($this->_tabs); // default
        }

        if ($has_pid && !isset($_REQUEST['t0']) && isset($_REQUEST['t'])) {
            $t = UIBase::GrabGoodInput('request', 't');
            if ($t != null) {
                $this->_tid = $t;
                $t1 = UIBase::GrabGoodInputWithReset('request', 't1');
                if ($t1 != null && ( $this->GetLast(self::FLD_TID) != $t1)) {
                    $this->_tid .= '`' . $t1;
				}

                if (($r = UIBase::GrabGoodInputWithReset('request', 'r')) != null) {
                    $this->_ref = $r;
				}
                if (($r1 = UIBase::GrabGoodInputWithReset('request', 'r1')) != null) {
                    $this->_ref .= '`' . $r1;
				}
            }
        }

        $this->_act = UIBase::GrabGoodInput("request", 'a');
        if ($this->_act == null) {
            $this->_act = 'v';
        }

        $tokenInput = UIBase::GrabGoodInput("request", 'tk');
        $this->_token = $_SESSION['token'];
        if ($this->_act != 'v' && $this->_token != $tokenInput) {
            die('Illegal entry point!');
        }

        if ($this->_act == 'B') {
            $this->TrimLastId();
            $this->_act = 'v';
        }

        $this->_sort = UIBase::GrabGoodInput("request", 'sort');

        $this->_allActions = [
			'a' => [DMsg::UIStr('btn_add'), 'fa-plus'],
			'v' => [DMsg::UIStr('btn_view'), 'fa-search-plus'],
			'E' => [DMsg::UIStr('btn_edit'), 'fa-edit'],
			's' => [DMsg::UIStr('btn_save'), 'fa-save'],
			'B' => [DMsg::UIStr('btn_back'), 'fa-reply'], //'fa-level-up'
			'n' => [DMsg::UIStr('btn_next'), 'fa-step-forward'],
			'd' => [DMsg::UIStr('btn_delete'), 'fa-trash-o'],
			'D' => [DMsg::UIStr('btn_delete'), 'fa-trash-o'],
			'C' => [DMsg::UIStr('btn_cancel'), 'fa-angle-double-left'],
			'i' => [DMsg::UIStr('btn_instantiate'), 'fa-cube'],
			'I' => [DMsg::UIStr('btn_instantiate'), 'fa-cube'],
			'X' => [DMsg::UIStr('btn_view'), 'fa-search-plus'],
		];
	}

    public function Get($field)
    {
        switch ($field) {
            case self::FLD_CtrlUrl:
				return sprintf('%sm=%s&p=%s', $this->_ctrlUrl, urlencode($this->_mid), $this->_pid);
            case self::FLD_View: return $this->_view;
            case self::FLD_ViewName: return $this->_viewName;
            case self::FLD_TopMsg: return $this->_topMsg;
            case self::FLD_ConfType: return $this->_confType;
            case self::FLD_ConfErr: return $this->_confErr;
            case self::FLD_MID: return $this->_mid;
            case self::FLD_PID: return $this->_pid;
            case self::FLD_TID: return $this->_tid;
            case self::FLD_REF: return $this->_ref;
            case self::FLD_ACT: return $this->_act;
            case self::FLD_PgData: return $this->_pageData;
            case self::FLD_ConfData: return $this->_confData;
            case self::FLD_TOKEN: return $this->_token;
            case self::FLD_SORT: return $this->_sort;
            case self::FLD_ICONTITLE:
                switch ($this->_view) {
                    case 'serv':
                        return ['fa-globe', DMsg::UIStr('menu_serv')];
                    case 'sl':
                        return ['fa-chain', DMsg::UIStr('menu_sl')];
                    case 'sl_':
                        return ['fa-chain', DMsg::UIStr('menu_sl_') . ' ' . $this->_viewName];
                    case 'vh':
                        return ['fa-cubes', DMsg::UIStr('menu_vh')];
                    case 'vh_':
                        return ['fa-cube', DMsg::UIStr('menu_vh_') . ' ' . $this->_viewName];
                    case 'tp':
                        return ['fa-files-o', DMsg::UIStr('menu_tp')];
                    case 'tp_':
                        return ['fa-files-o', DMsg::UIStr('menu_tp_') . ' ' . $this->_viewName];
                    case 'admin':
                        return ['fa-gear', DMsg::UIStr('menu_webadmin')];
                    case 'al':
                        return ['fa-chain', DMsg::UIStr('menu_webadmin') . ' - ' . DMsg::UIStr('menu_sl')];
                    case 'al_':
                        return ['fa-chain', DMsg::UIStr('menu_webadmin') . ' - ' . DMsg::UIStr('menu_sl_') . ' ' . $this->_viewName];
                }
                break;
            default: error_log("invalid DInfo field : $field\n");
        }
    }

    public function Set($field, $value)
    {
        switch ($field) {
            case self::FLD_ConfErr:
                $this->_confErr = $value;
                break;
            case self::FLD_ACT:
                $this->_act = $value;
                break;
            case self::FLD_PgData:
                $this->_pageData = $value;
                break;
            case self::FLD_ConfData:
                $this->_confData = $value;
                break;
            case self::FLD_ServData:
                $this->_servData = $value;
                break;
            case self::FLD_REF:
                $this->_ref = $value;
                break;
            case self::FLD_ViewName:
                $this->_viewName = $value;
                if ($value == null) { // by delete
                    $value = '';
				} else {
                    $value = '_' . $value;
				}

                if (($pos = strpos($this->_mid, '_')) > 0) {
                    $this->_mid = substr($this->_mid, 0, $pos) . $value;
                }
                break;
            case self::FLD_TopMsg:
                $this->_topMsg[] = $value;
                break;
            default: die("not supported - $field");
        }
    }

    public function SetPrintingLinked($printinglinked)
    {
        $this->_isPrintingLinkedTbl = $printinglinked;
    }

    public function InitUIProps($props)
    {
        $props->Set(UIProperty::FLD_FORM_HIDDENVARS, [
            'a'  => 'v',
            'm'  => $this->_mid,
            'p'  => $this->_pid,
            't'  => $this->_tid,
            'r'  => $this->_ref,
            'tk' => $this->_token]);

        if ($this->_servData != null) {
            $props->Set(UIProperty::FLD_SERVER_NAME, $this->_servData->GetId());
        }

        $uri = $this->_ctrlUrl . 'm=' . urlencode($this->_mid);

        $tabs = [];
        $uri .= '&p=';

        foreach ($this->_tabs as $pid => $tabName) {
            if ($pid == $this->_pid) {
                //$bread[$tabName] = $uri . $pid;
                $name = '1' . $tabName;
            } else {
                $name = '0' . $tabName;
            }
            //$tabs[$name] = $uri . $pid;
            $tabs[$name] = "javascript:lst_conf('v', '$pid','-','-')";
        }
        $props->Set(UIProperty::FLD_TABS, $tabs);
    }

    public function IsViewAction()
    {
        //$viewTags = 'vsDdBCiI';
        $editTags = 'EaScn';
        return ( strpos($editTags, $this->_act) === false );
    }

    public function GetActionData($actions, $editTid = '', $editRef = '', $addTid = '')
    {
        $actdata = [];

        $chars = preg_split('//', $actions, -1, PREG_SPLIT_NO_EMPTY);

        $cur_tid = $this->GetLast(self::FLD_TID);
        foreach ($chars as $act) {
            $name = $this->_allActions[$act][0];
            $ico = $this->_allActions[$act][1];
            if ($act == 'C') {
                $act = 'B';
            }

            if ($act == 'B' && $this->_isPrintingLinkedTbl) {
                continue; // do not print Back for linked view
			}

            if ($act == 's' || $act == 'n') {
                $href = "javascript:lst_conf('$act','','','')";
            } elseif ($act == 'X') {
                //vhtop=>vh_... tptop=>tp_.... sltop=>sl_...
                $href = $this->_ctrlUrl . 'm=' . urlencode($this->_view . '_' . $editRef);
                $act = 'v';
            } else {

                if ($act == 'a') {
                    $edittid = $addTid;
                    $editref = '~';
                } else {
                    $edittid = $editTid;
                    $editref = $editRef;
                }

                if ($edittid == '' || $edittid == $cur_tid) {
                    $t = $this->_tid;
                } elseif ($cur_tid != null && $cur_tid != $edittid) {
                    $t = $this->_tid . '`' . $edittid;
                } else {
                    $t = $edittid;
                }

                if ($editref == '') {
                    $r = $this->_ref;
                } elseif ($this->_ref != null && $this->_ref != $editref) {
                    $r = $this->_ref . '`' . $editref;
                } else {
                    $r = $editref;
                }
				if ($t) {
					$t = addslashes($t);
				}
				if ($r) {
					$r = addslashes($r);
				}

                $href = "javascript:lst_conf('$act', '', '$t', '$r')";
            }
            $actdata[$act] = ['label' => $name, 'href' => $href, 'ico' => $ico];
        }
        return $actdata;
    }

    public function TrimLastId()
    {
        if ($this->_tid && ($pos = strrpos($this->_tid, '`')) !== false) {
            $this->_tid = substr($this->_tid, 0, $pos);
		} else {
            $this->_tid = null;
		}

        if ($this->_ref && ($pos = strrpos($this->_ref, '`')) !== false) {
            $this->_ref = substr($this->_ref, 0, $pos);
		} elseif ($this->_view == 'sl_' || $this->_view == 'al_' || $this->_pid == 'base' || $this->_pid == 'mbr') {
            $this->_ref = $this->_viewName; // still in serv conf
		} else {
            $this->_ref = null;
		}
    }

    public function GetLast($field)
    {
        $id = null;
        if ($field == self::FLD_TID) {
            $id = $this->_tid;
		} elseif ($field == self::FLD_REF) {
            $id = $this->_ref;
		}

        if ($id != null && ($pos = strrpos($id, '`')) !== false) {
            if (strlen($id) > $pos + 1) {
                $id = substr($id, $pos + 1);
			} else {
                $id = '';
			}
		}

        return $id;
    }

    public function GetFirst($field)
    {
        $id = null;
        if ($field == self::FLD_TID) {
            $id = $this->_tid;
		} elseif ($field == self::FLD_REF) {
            $id = $this->_ref;
		}

        if ($id != null && ($pos = strpos($id, '`')) !== false) {
            $id = substr($id, 0, $pos);
        }

        return $id;
    }

    public function GetParentRef()
    {
        if ($this->_ref && ($pos = strrpos($this->_ref, '`')) !== false) {
            return substr($this->_ref, 0, $pos);
		}
		return '';
    }

    public function SwitchToSubTid($extracted)
    {
        if ($this->_tid && ($pos = strrpos($this->_tid, '`')) !== false) {
            $tid0 = substr($this->_tid, 0, $pos + 1);
            $tid = substr($this->_tid, $pos + 1);
        } else {
            $tid0 = '';
            $tid = $this->_tid;
        }
        $tbl = DTblDef::getInstance()->GetTblDef($tid);

        $subtbls = $tbl->Get(DTbl::FLD_SUBTBLS);
        $newkey = $extracted->GetChildVal($subtbls[0]);
        $subtid = '';
        if ($newkey != null) {
            if ($newkey == '0' || !isset($subtbls[$newkey])) {
                $subtid = $subtbls[1];
			} else {
                $subtid = $subtbls[$newkey];
			}
        }

        $this->_tid = $tid0 . $subtid;
    }

    public function GetDerivedSelOptions($tid, $loc, $node)
    {

        if (substr($loc, 0, 13) == 'extprocessor:') {
			return $this->getDerivedSelOptions_extprocessor($tid, $loc, $node);
        }

        if (in_array($loc, ['virtualhost', 'listener', 'module'])) {
            $names = $this->_servData->GetChildrenValues($loc);
        } elseif ($loc == 'realm') {
            if ($this->_view == DInfo::CT_TP) {
                $loc = "virtualHostConfig:$loc";
			}
            $names = $this->_confData->GetChildrenValues($loc);
        }

        sort($names);
        $o = [];
        foreach ($names as $name) {
            $o[$name] = $name;
		}

        return $o;
    }

	protected function getDerivedSelOptions_extprocessor($tid, $loc, $node)
	{
        $o = [];
        // substr($loc, 0, 13) == 'extprocessor:')
		$type = substr($loc, 13);
		if ($type == '$$type') {
			if ($node != null) {
				$type = $node->GetChildVal('type');
			}
			if ($type == null) { // default
				$type = 'fcgi';
			}
		}
		if ($type == 'cgi') {
			$o['cgi'] = 'CGI Daemon';
			return $o;
		}
		if ($type == 'module') {
			$modules = $this->_servData->GetChildrenValues('module');
			if ($modules != null) {
				foreach ($modules as $mn) {
					$o[$mn] = $mn;
				}
			}
			return $o;
		}

		$exps = [];
		if (($servexps = $this->_servData->GetRootNode()->GetChildren('extprocessor')) != null) {
			if (is_array($servexps)) {
				foreach ($servexps as $exname => $ex) {
					if ($ex->GetChildVal('type') == $type) {
						$exps[] = $exname;
					}
				}
			}
			elseif ($servexps->GetChildVal('type') == $type) {
				$exps[] = $servexps->Get(CNode::FLD_VAL);
			}
		}

		if ($this->_view == DInfo::CT_SERV) {
			foreach ($exps as $exname) {
				$o[$exname] = $exname;
			}
			return $o;
		}

		foreach ($exps as $exname) {
			$o[$exname] = '[' . DMsg::UIStr('note_serv_level') . "]: $exname";
		}

		$loc = ($this->_view == DInfo::CT_TP) ? 'virtualHostConfig:extprocessor' : 'extprocessor';
		if (($vhexps = $this->_confData->GetRootNode()->GetChildren($loc)) != null) {
			if (is_array($vhexps)) {
				foreach ($vhexps as $exname => $ex) {
					if ($ex->GetChildVal('type') == $type) {
						$o[$exname] = '[' . DMsg::UIStr('note_vh_level') . "]: $exname";
					}
				}
			}
			elseif ($vhexps->GetChildVal('type') == $type) {
				$exname = $vhexps->Get(CNode::FLD_VAL);
				$o[$exname] = '[' . DMsg::UIStr('note_vh_level') . "]: $exname";
			}
		}
		return $o;
	}

    public function GetVHRoot()
    {
        // type = 'SR', 'VR'
        if ($this->_view == self::CT_VH) {
            $vn = $this->_viewName;
            if (($vh = $this->_servData->GetChildNodeById('virtualhost', $vn)) != null) {
                $vhroot = PathTool::GetAbsFile($vh->GetChildVal('vhRoot'), 'SR', $vn);
                return $vhroot;
            }
        }
        return null;
    }

}
