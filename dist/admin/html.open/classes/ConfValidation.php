<?php

class ConfValidation
{
	var $_info;

	public function __construct() {
		$this->_info = NULL;
	}
	
	function validateConf($confData, $info)
	{
		$this->_info = $info;
		$type = $confData->_type;

		$pages = DPageDef::GetInstance()->GetFileDef($type);

		for ( $c = 0 ; $c < count($pages) ; ++$c ) {
			$page = $pages[$c];
			if ( $page->GetDataLoc() == NULL ) {
				$this->validateElement($page->GetTids(), $confData->_data);
			} else {
				$data = &DUtil::locateData( $confData->_data, $page->GetDataLoc() );
				if ( $data == NULL || count($data) == 0 )
				continue;
				$keys = array_keys($data);
				foreach( $keys as $key ) {
					$this->validateElement($page->GetTids(), $data[$key] );
				}
			}
		}

		if ( $type == 'serv' || $type == 'admin' ) {
			$this->checkListeners($confData);
		}
		

		$this->_info = NULL;
	}

	public function ExtractPost($tbl, &$d, $disp)
	{
		$this->_info = $disp->_info;
		$goFlag = 1 ;
		$index = array_keys($tbl->_dattrs);
		foreach ( $index as $i ) {
			$attr = $tbl->_dattrs[$i];

			if ( $attr == NULL ) 
				continue;

			if ( $attr->_FDE[2] == 'N' || $attr->blockedVersion())	
				continue;

				
			$d[$attr->_key] = $attr->extractPost();
			$needCheck = TRUE;
			if ( $attr->_type == 'sel1' || $attr->_type == 'sel2' )	{
				if ( $disp->_act == 'c' ) {
					$needCheck = FALSE;
				} 
				else {
					$tbl->populate_sel1_options($this->_info, $d, $attr);
				}
			}

			if ( $needCheck ) {
				$res = $this->validateAttr($attr, $d[$attr->_key]);
				$this->setValid($goFlag, $res);
			}
		}

		$res = $this->validatePostTbl($tbl, $d);

		$this->setValid($goFlag, $res);

		$this->_info = NULL;
		
		// if 0 , make it always point to curr page
		return $goFlag;
	}

	//private:
	private function checkListeners($confData)
	{
		if ( isset($confData->_data['listeners']) )	{
			$keys = array_keys($confData->_data['listeners']);
			foreach ( $keys as $key ) {
				$this->checkListener( $confData->_data['listeners'][$key] );
			}
		}
	}
	
	public function CheckListener(&$listener)
	{
		if ( $listener['secure']->GetVal() == '0' ) {
			if ( isset($listener['certFile']) && !$listener['certFile']->HasVal() ) {
				$listener['certFile']->SetErr(NULL);
			}
			if ( isset($listener['keyFile']) && !$listener['keyFile']->HasVal() ) {
				$listener['keyFile']->SetErr(NULL);
			}
		} else {
			$tids = array('L_CERT');
			$this->validateElement($tids, $listener);
		}
	}

	private function validateElement($tids, &$data)
	{
		$tblDef = DTblDef::GetInstance();
		$valid = 1;
		foreach ( $tids as $tid ) {
			$tbl = $tblDef->GetTblDef($tid);
			$d = &DUtil::locateData( $data, $tbl->_dataLoc );

			if ( $d == NULL ) continue;

			if ( $tbl->_holderIndex != NULL ) {
				$keys = array_keys( $d );
				foreach( $keys as $key ) {
					$res = $this->validateTblAttr($tblDef, $tbl, $d[$key]);
					$this->setValid($valid, $res);
				}
			} else {
				$res = $this->validateTblAttr($tblDef, $tbl, $d);
				$this->setValid($valid, $res);
			}
		}
		return $valid;
	}

	private function setValid(&$valid, $res)
	{
		if ( $valid != -1 )	{
			if ( $res == -1 ) {
				$valid = -1;
			} elseif ( $res == 0 && $valid == 1 ) {
				$valid = 0;
			}
		}
		if ( $res == 2 ) {
			$valid = 2;
		}
	}

	private function validatePostTbl($tbl, &$data)
	{
		$isValid = 1;
		if ( $tbl->_holderIndex != NULL && isset($data[$tbl->_holderIndex])) {
			$newref = $data[$tbl->_holderIndex]->GetVal();
			$oldref = NULL;

			if(isset($this->_info['holderIndex_cur'])) {
				$oldref = $this->_info['holderIndex_cur'];
			}
			//echo "oldref = $oldref newref = $newref \n";
			if ( $oldref == NULL || $newref != $oldref ) {
				if (isset($this->_info['holderIndex']) && $this->_info['holderIndex'] != NULL
				&& in_array($newref, $this->_info['holderIndex']) ) {
					$data[$tbl->_holderIndex]->SetErr('This value has been used! Please choose a unique one.');
					$isValid = -1;
				}
			}
		}
		
		$checkedTids = array('VH_TOP_D','VH_BASE','VH_UDB', 'VH_SECHL',
		'ADMIN_USR', 'ADMIN_USR_NEW', 
		'L_GENERAL', 'L_GENERAL1', 'ADMIN_L_GENERAL', 'ADMIN_L_GENERAL1', 
		'L_SSL', 'VH_SSL_SSL', 'TP', 'TP1');

		if ( in_array($tbl->_id, $checkedTids) ) {
			$funcname = 'chkPostTbl_' . $tbl->_id;
			$res = $this->$funcname($data);
			$this->setValid($isValid, $res);
		}
		return $isValid;
	}
	
	
	private function chkPostTbl_TP(&$d)
	{
		$isValid = 1;
		
		$confCenter = ConfCenter::singleton();
		
		$oldName = trim($confCenter->GetDispInfo()->_name);
		$newName = trim($d['name']->GetVal());
		 
		if($oldName != $newName && array_key_exists($newName, $confCenter->_serv->_data['tpTop'])) {
			$d['name']->SetErr("Template: \"$newName\" already exists. Please use a different name.");
			$isValid = -1;
			
		}
		
		return $isValid;
	}
	
	private function chkPostTbl_TP1(&$d)
	{
		return $this->chkPostTbl_TP($d);
	}
	
	
	private function chkPostTbl_VH_TOP_D(&$d)
	{
		return $this->chkPostTbl_VH_BASE($d);
	}
	
	private function chkPostTbl_VH_BASE(&$d)
	{
		$isValid = 1;
		
		$confCenter = ConfCenter::singleton();
		
		$oldName = trim($confCenter->GetDispInfo()->_name);
		$newName = trim($d['name']->GetVal());
		 
		if($oldName != $newName && array_key_exists($newName, $confCenter->_serv->_data['vhTop'])) {
			$d['name']->SetErr("Virtual Hostname: \"$newName\" already exists. Please use a different name.");
			$isValid = -1;
			
		}
		
		return $isValid;
	}
	
	private function chkPostTbl_VH_UDB(&$d)
	{
		$isValid = 1;
		if ( $d['pass']->GetVal() != $d['pass1']->GetVal() ) {
			$d['pass']->SetErr('Passwords do not match!');
			$isValid = -1;
		}

		if ( !$d['pass']->HasVal() ) { //new user
			$d['pass']->SetErr('Missing password!');
			$isValid = -1;
		}

		if ( $isValid == -1 ) {
			return -1;
		}

		if ( strlen($d['pass']->GetVal()) > 0 ) {
			$newpass = $this->encryptPass($d['pass']->GetVal());
			$d['passwd'] = new CVal($newpass);
		}
		return 1;
	}

	private function chkPostTbl_VH_SECHL(&$d)
	{
		$isValid = 1;
		if ( $d['enableHotlinkCtrl']->GetVal() == '0' ) {
			if ( isset($d['suffixes']) && !$d['suffixes']->HasVal() ) {
				$d['suffixes']->SetErr(NULL);
			}
			if ( isset($d['allowDirectAccess']) && !$d['allowDirectAccess']->HasVal() ) {
				$d['allowDirectAccess']->SetErr(NULL);
			}
			if ( isset($d['onlySelf']) && !$d['onlySelf']->HasVal() ) {
				$d['onlySelf']->SetErr(NULL);
			}
			$isValid = 2;
		} else {
			if ( $d['onlySelf']->GetVal() == '0'
			&&  !$d['allowedHosts']->HasVal() ) {
				$d['allowedHosts']->SetErr('Must be specified if "Only Self Reference" is set to No');
				$isValid = -1;
			}
		}
		return $isValid;
	}

	private function encryptPass($val)
	{
		$valid_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/.";
		if (CRYPT_MD5 == 1) 
		{
		    $salt = '$1$';
		    for($i = 0; $i < 8; $i++) 
		    {
			$salt .= $valid_chars[rand(0,strlen($valid_chars)-1)];
		    }
		    $salt .= '$';
		}
		else
		{
		    $salt = $valid_chars[rand(0,strlen($valid_chars)-1)];
		    $salt .= $valid_chars[rand(0,strlen($valid_chars)-1)];
		}		
		$pass = crypt($val, $salt);
		return $pass;
	}

	private function chkPostTbl_ADMIN_USR(&$d)
	{
		$isValid = 1;
		if ( !$d['oldpass']->HasVal() ) {
			$d['oldpass']->SetErr('Missing Old password!');
			$isValid = -1;
		} else {
			$file = $_SERVER['LS_SERVER_ROOT'] . 'admin/conf/htpasswd';
			$udb = ConfigFileEx::loadUserDB($file);
			$olduser = $this->_info['holderIndex_cur'];
			$passwd = $udb[$olduser]['passwd']->GetVal();

			$oldpass = $d['oldpass']->GetVal();
			$encypt = crypt($oldpass, $passwd);

			if ( $encypt != $passwd ) {
				$d['oldpass']->SetErr('Invalid old password!');
				$isValid = -1;
			}
		}

		if ( !$d['pass']->HasVal() )	{
			$d['pass']->SetErr('Missing new password!');
			$isValid = -1;
		} elseif ( $d['pass']->GetVal() != $d['pass1']->GetVal() ) {
			$d['pass']->SetErr('New passwords do not match!');
			$isValid = -1;
		}

		if ( $isValid == -1 ) {
			return -1;
		}

		$newpass = $this->encryptPass($d['pass']->GetVal());
		$d['passwd'] = new CVal($newpass);

		return 1;
	}


	private function chkPostTbl_ADMIN_USR_NEW(&$d)
	{
		$isValid = 1;
		if ( !$d['pass']->HasVal() )	{
			$d['pass']->SetErr('Missing new password!');
			$isValid = -1;
		} elseif ( $d['pass']->GetVal() != $d['pass1']->GetVal() ) {
			$d['pass']->SetErr('New passwords do not match!');
			$isValid = -1;
		}

		if ( $isValid == -1 ) {
			return -1;
		}

		$newpass = $this->encryptPass($d['pass']->GetVal());
		$d['passwd'] = new CVal($newpass);

		return 1;
	}

	
	private function chkPostTbl_L_GENERAL(&$d)
	{
		$isValid = 1;
		
		$ip = $d['ip']->GetVal();
		if ( $ip == 'ANY' ) {
			$ip = '*';
		}
		$port = $d['port']->GetVal();
		$d['address'] = new CVal("$ip:$port");
		
		$confCenter = ConfCenter::singleton();
		
		$oldName = trim($confCenter->GetDispInfo()->_name);
		$newName = trim($d['name']->GetVal());

		if($oldName != $newName && array_key_exists($newName, $confCenter->_serv->_data['listeners'])) {
			$d['name']->SetErr("Listener \"$newName\" already exists. Please use a different name.");
			$isValid = -1;
		}
		
		return $isValid;
	}

	private function chkPostTbl_L_GENERAL1(&$d)
	{
		return $this->chkPostTbl_L_GENERAL($d);
	}

	private function chkPostTbl_ADMIN_L_GENERAL(&$d)
	{
		return $this->chkPostTbl_L_GENERAL($d);
	}
	
	private function chkPostTbl_ADMIN_L_GENERAL1(&$d)
	{
		return $this->chkPostTbl_L_GENERAL($d);
	}
	
	private function chkPostTbl_L_SSL(&$d)
	{
		/* array( 'SSLv3' => ' ', 'TLSv1' => ' ',
		'HIGH' => ' ', 'MEDIUM' => ' ', 'LOW' => ' ',
		'EXPORT56' => ' ', 'EXPORT40' => ' ', 'eNULL' => ' ');*/
		$ciphers = '';

		if (isset($_POST['ck1'])) {
			$ciphers .= 'SSLv3:';
		}
		if (isset($_POST['ck2'])) {
			$ciphers .= 'TLSv1:';
		}
		if (isset($_POST['ck3'])) {
			$ciphers .= 'HIGH:';
		}
		if (isset($_POST['ck4'])) {
			$ciphers .= 'MEDIUM:';
		}
		if (isset($_POST['ck5'])) {
			$ciphers .= 'LOW:';
		}
		if (isset($_POST['ck6'])) {
			$ciphers .= 'EXPORT56:';
		}
		if (isset($_POST['ck7'])) {
			$ciphers .= 'EXPORT40:';
		}
		if ($ciphers == '') {
			$d['ciphers'] = new CVal('', 'Need to select at least one');
			return -1;
		}

		$ciphers .= '!aNULL:!MD5:!SSLv2:!eNULL:!EDH';
		$d['ciphers'] = new CVal($ciphers);
		return 1;
	}

	private function chkPostTbl_VH_SSL_SSL(&$d)
	{
		return $this->chkPostTbl_L_SSL($d);
	}
	
	private function validateTblAttr($tblDef, $tbl, &$data)
	{
		$valid = 1;
		if ( $tbl->_subTbls != NULL ) {
			$tid = DUtil::getSubTid($tbl->_subTbls, $data);
			if ( $tid == NULL ) {
				return;
			}
			$tbl1 = $tblDef->GetTblDef($tid);
		} else {
			$tbl1 = &$tbl;
		}

		$index = array_keys($tbl1->_dattrs);
		foreach ( $index as $i ) {
			$attr = $tbl1->_dattrs[$i];

			if ( $attr->_type == 'sel1' || $attr->_type == 'sel2' ) {
				$tbl->populate_sel1_options($this->_info, $data, $attr);
			}

			$res = $this->validateAttr($attr, $data[$attr->_key]);
			$this->setValid($valid, $res);
		}
		return $valid;
	}

	private function validateAttr($attr, &$cvals)
	{
		if ( $attr->_type == 'cust' ) {
			return 1;
		}

		$valid = 1;
		if ( is_array($cvals) )	{
			for ( $i = 0 ; $i < count($cvals) ; ++$i ) {
				$res = $this->isValidAttr($attr, $cvals[$i]);
				$this->setValid($valid, $res);
			}
		} else {
			$valid = $this->isValidAttr($attr, $cvals);
		}
		return $valid;
	}

	private function isValidAttr($attr, $cval)
	{
		if ($cval == NULL)
			return -1;
			
		$cval->SetErr(NULL);

		if ( !$cval->HasVal()) {
			if ( $attr->_allowNull ) {
				return 1;
			}
			$cval->SetErr('value must be set');
			return -1;
		}

		$chktype = array('uint', 'name', 'vhname', 'sel','sel1','sel2',
		'bool','file','filep','file0','file1', 'filetp', 'path',
		'uri','expuri','url', 'email', 'dir', 'addr', 'parse');
		
		if ( !in_array($attr->_type, $chktype) )	{
			// not checked type ('domain', 'subnet'
			return 1;
		}

		$type3 = substr($attr->_type, 0, 3);
		if ( $type3 == 'sel' ) {
			// for sel, sel1, sel2
			$funcname = 'chkAttr_sel';
		}
		elseif ( $type3 == 'fil' || $type3 == 'pat' ) {
			$funcname = 'chkAttr_file';
		}
		else {
			$funcname = 'chkAttr_' . $attr->_type;
		}

		if ( $attr->_multiInd == 1 ) {
			$valid = 1;
			$vals = DUtil::splitMultiple($cval->GetVal());
			$err = array();
			$funcname .= '_val';
			foreach( $vals as $i=>$v ) {
				$res = $this->$funcname($attr, $v, $err[$i]);
				$this->setValid($valid, $res);
			}
			$cval->SetErr(trim(implode(' ', $err)));
			return $valid;
		}else {
			return $this->$funcname($attr, $cval);
		}
	}

	private function chkAttr_sel($attr, $cval)
	{
		$err = '';
		$res = $this->chkAttr_sel_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}

	private function chkAttr_sel_val($attr, $val, &$err)
	{
		if ( isset( $attr->_maxVal ) 
			&& !array_key_exists($val, $attr->_maxVal) ) {
				$err = "invalid value: $val";
			return -1;
		}
		return 1;
	}
	
	private function chkAttr_name($attr, $cval)
	{
		$cval->SetVal( preg_replace("/\s+/", ' ', $cval->GetVal()));
		$res = $this->chkAttr_name_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}

	private function chkAttr_name_val($attr, $val, &$err)
	{
		if ( preg_match( "/[<>&%]/", $val) ) {
			$err = 'invalid characters in name';
			return -1;
		}
		if ( strlen($val) > 100 ) {
			$err = 'name can not be longer than 100 characters';
			return -1;
		}
		return 1;
	}
	
	private function chkAttr_vhname($attr, $cval)
	{
		$cval->SetVal(preg_replace("/\s+/", ' ', $cval->GetVal()));
		$val = $cval->GetVal();
		if ( preg_match( "/[,;<>&%]/", $val ) ) {
			$cval->SetErr('Invalid characters found in name');
			return -1;
		}
		if ( strpos($val, ' ') !== FALSE ) {
			$cval->SetErr('No space allowed in the name');
			return -1;
		}
		if ( strlen($val) > 100 ) {
			$cval->SetErr('name can not be longer than 100 characters');
			return -1;
		}
		$this->_info['VH_NAME'] = $val;
		return 1;
	}

	private function allow_create($attr, $absname)
	{
		if ( strpos($attr->_maxVal, 'c') === FALSE ) {
			return FALSE;
		}
		if ( $attr->_minVal >= 2
		&& ( strpos($absname, $_SERVER['LS_SERVER_ROOT'])  === 0 )) {
			return TRUE;
		}

		if (isset($this->_info['VH_ROOT'])) {
			$VH_ROOT = $this->_info['VH_ROOT'];
		} else {
			$VH_ROOT = NULL;
		}

		if (isset($this->_info['DOC_ROOT'])) {
			$DOC_ROOT = $this->_info['DOC_ROOT'];
		}

		if ( $attr->_minVal >= 3 && ( strpos($absname, $VH_ROOT) === 0 ) ) {
			return TRUE;
		}

		if ( $attr->_minVal == 4 && ( strpos($absname, $DOC_ROOT) === 0 ) ) {
			return TRUE;
		}

		return FALSE;
	}

	private function test_file(&$absname, &$err, $attr)
	{
		$absname = PathTool::clean($absname);
		if ( isset( $_SERVER['LS_CHROOT'] ) )	{
			$root = $_SERVER['LS_CHROOT'];
			$len = strlen($root);
			if ( strncmp( $absname, $root, $len ) == 0 ) {
				$absname = substr($absname, $len);
			}
		}

		if ( $attr->_type == 'file0' ) {
			if ( !file_exists($absname) ) {
				return 1; //allow non-exist
			}
		}
		if ( $attr->_type == 'path' || $attr->_type == 'filep' || $attr->_type == 'dir' ) {
			$type = 'path';
		} else {
			$type = 'file';
		}

		if ( ($type == 'path' && !is_dir($absname))
		|| ($type == 'file' && !is_file($absname)) ) {
			$err = $type .' '. htmlspecialchars($absname) . ' does not exist.';
			if ( $this->allow_create($attr, $absname) ) {
				$err .= ' <a href="javascript:createFile(\''. $attr->_htmlName . '\')">CLICK TO CREATE</a>';
			} else {
				$err .= ' Please create manually.';
			}

			return -1;
		}
		if ( (strpos($attr->_maxVal, 'r') !== FALSE) && !is_readable($absname) ) {
			$err = $type . ' '. htmlspecialchars($absname) . ' is not readable';
			return -1;
		}
		if ( (strpos($attr->_maxVal, 'w') !== FALSE) && !is_writable($absname) ) {
			$err = $type . ' '. htmlspecialchars($absname) . ' is not writable';
			return -1;
		}
		if ( (strpos($attr->_maxVal, 'x') !== FALSE) && !is_executable($absname) ) {
			$err = $type . ' '. htmlspecialchars($absname) . ' is not executable';
			return -1;
		}
		return 1;
	}

	private function chkAttr_file($attr, $cval)
	{
		$val = $cval->GetVal();
		$err = '';
		$res = $this->chkAttr_file_val($attr, $val, $err);
		$cval->SetVal($val);
		$cval->SetErr($err);
		return $res;
	}
	
	private function chkAttr_dir($attr, $cval)
	{
		$val = $cval->GetVal();
		$err = '';
		
		if ( substr($val,-1) == '*' ) {
			$res = $this->chkAttr_file_val($attr, substr($val,0,-1), $err);
		} else {
			$res = $this->chkAttr_file_val($attr, $val, $err);
		}
		$cval->SetVal($val);
		$cval->SetErr($err);
		return $res;
	}

	public function chkAttr_file_val($attr, $val, &$err)
	{
		//this is public	
		clearstatcache();
		$err = NULL;

		if ( $attr->_type == 'filep' ) {
			$path = substr($val, 0, strrpos($val,'/'));
		} else {
			$path = $val;
			if ( $attr->_type == 'file1' ) {
				$pos = strpos($val, ' ');
				if ( $pos > 0 ) {
					$path = substr($val, 0, $pos);
				}
			}
		}

		$res = $this->chk_file1($attr, $path, $err);
		if ($attr->_type == 'filetp') {
			$pathtp = $_SERVER['LS_SERVER_ROOT'] . 'conf/templates/';
			if (strstr($path, $pathtp) === FALSE) {
				$err = ' Template file must locate within $SERVER_ROOT/conf/templates/';
				$res = -1;
			}
			else if (substr($path, -4) != '.xml') {
				$err = ' Template file name needs to be ".xml"';
				$res = -1;
			}
		}
		if ( $res == -1
		&& $_POST['file_create'] == $attr->_htmlName
		&& $this->allow_create($attr, $path) )	{
			if ( PathTool::createFile($path, $err, $attr->_htmlName) ) {
				$err = "$path has been created successfully.";
			}
			$res = 0; // make it always point to curr page
		}
		if ( $attr->_key == 'vhRoot' )	{
			if ( substr($path,-1) != '/' ) {
				$path .= '/';
			}
			if ($res == -1) {
				// do not check path for vhroot, it may be different owner
				$err = '';
				$res = 1;
			}
			$this->_info['VH_ROOT'] = $path;
		}
		elseif ($attr->_key == 'docRoot') {
			if ($res == -1) {
				// do not check path for vhroot, it may be different owner
				$err = '';
				$res = 1;
			}
		}
		return $res;
	}

	private function chk_file1($attr, &$path, &$err)
	{
		if(!strlen($path)) {
			$err = "Invalid Path.";
			return -1;
		}
		
		$s = $path{0};
		
		if ( strpos($path, '$VH_NAME') !== FALSE )	{
			$path = str_replace('$VH_NAME', $this->_info['VH_NAME'], $path);
		}

		if ( $s == '/' ) {
			return $this->test_file($path, $err, $attr);
		}

		if ( $attr->_minVal == 1 ) {
			$err = 'only accept absolute path';
			return -1;
		}
		elseif ( $attr->_minVal == 2 ) {
			if ( strncasecmp('$SERVER_ROOT', $path, 12) != 0 )	{
				$err = 'only accept absolute path or path relative to $SERVER_ROOT' . $path;
				return -1;
			} else {
				$path = $_SERVER['LS_SERVER_ROOT'] . substr($path, 13);
			}
		}
		elseif ( $attr->_minVal == 3 ) {
			if ( strncasecmp('$SERVER_ROOT', $path, 12) == 0 ) {
				$path = $_SERVER['LS_SERVER_ROOT'] . substr($path, 13);
			} elseif ( strncasecmp('$VH_ROOT', $path, 8) == 0 )	{
				if (isset($this->_info['VH_ROOT'])) {
					$path = $this->_info['VH_ROOT'] . substr($path, 9);
				}
			} else {
				$err = 'only accept absolute path or path relative to $SERVER_ROOT or $VH_ROOT';
				return -1;
			}
		}
		elseif ( $attr->_minVal == 4 ) {
			if ( strncasecmp('$SERVER_ROOT', $path, 12) == 0 ) {
				$path = $_SERVER['LS_SERVER_ROOT'] . substr($path, 13);
			} elseif ( strncasecmp('$VH_ROOT', $path, 8) == 0 )	{
				$path = $this->_info['VH_ROOT'] . substr($path, 9);
			} elseif ( strncasecmp('$DOC_ROOT', $path, 9) == 0 ) {
				$path = $this->_info['DOC_ROOT'] . substr($path, 10);
			} else {
				$path = $this->_info['DOC_ROOT'] . $path;
			}
		}

		return $this->test_file($path, $err, $attr);
	}

	private function chkAttr_uri($attr, $cval)
	{
		$val = $cval->GetVal();
		if ( $val[0] != '/' ) {
			$cval->SetErr('URI must start with "/"');
			return -1;
		}
		return 1;
	}

	private function chkAttr_expuri($attr, $cval)
	{
		$val = $cval->GetVal();
		if ( $val{0} == '/' || strncmp( $val, 'exp:', 4 ) == 0 ) {
			return 1;
		} else {
			$cval->SetErr('URI must start with "/" or "exp:"');
			return -1;
		}
	}

	private function chkAttr_url($attr, $cval)
	{
		$val = $cval->GetVal();
		if (( $val{0} != '/' )
		&&( strncmp( $val, 'http://', 7 ) != 0 )
		&&( strncmp( $val, 'https://', 8 ) != 0 )) {
			$cval->SetErr('URL must start with "/" or "http(s)://"');
			return -1;
		}
		return 1;
	}

	private function chkAttr_email($attr, $cval)
	{
		$err = '';
		$res = $this->chkAttr_email_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}
	
	private function chkAttr_email_val($attr, $val, &$err)
	{
		if ( preg_match("/^[[:alnum:]._-]+@.+/", $val ) ) {
			return 1;
		} else {
			$err = 'invalid email format: ' . $val;
			return -1;
		}
	}
	
	private function chkAttr_addr($attr, $cval)
	{
		if ( preg_match("/^[[:alnum:]._-]+:(\d)+$/", $cval->GetVal()) ) {
			return 1;
		} elseif ( preg_match("/^UDS:\/\/.+/i", $cval->GetVal()) ) {
			return 1;
		} else {
			$cval->SetErr('invalid address: correct syntax is "IPV4_address:port" or UDS://path');
			return -1;
		}
	}

	private function chkAttr_bool($attr, $cval)
	{
		$val = $cval->GetVal();
		if ( $val === '1' || $val === '0' ) {
			return 1;
		}
		$cval->SetErr('invalid value');
		return -1;
	}

	private function chkAttr_parse($attr, $cval)
	{
		$err = '';
		$res = $this->chkAttr_parse_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}
	
	private function chkAttr_parse_val($attr, $val, &$err)
	{
		if ( preg_match($attr->_minVal, $val) ) {
			return 1;
		} else {
			$err = "invalid format - $val, syntax is {$attr->_maxVal}";
			return -1;
		}
	}

	private function getKNum($strNum)
	{
		$tag = substr($strNum, -1);
		switch( $tag ) {
			case 'K':
			case 'k': $multi = 1024; break;
			case 'M':
			case 'm': $multi = 1048576; break;
			case 'G':
			case 'g': $multi = 1073741824; break;
			default: return intval($strNum);
		}

		return (intval(substr($strNum, 0, -1)) * $multi);
	}

	private function chkAttr_uint($attr, $cval)
	{
		$val = $cval->GetVal();
		if ( preg_match("/^(-?\d+)([KkMmGg]?)$/", $val, $m) ) {
			$val1 = $this->getKNum($val);
			if (isset($attr->_minVal)) {
				$min = $this->getKNum($attr->_minVal);
				if ($val1 < $min) {
					$cval->SetErr('number is less than the minumum required');
					return -1;
				}

			}
			if (isset($attr->_maxVal)) {
				$max = $this->getKNum($attr->_maxVal);
				if ( $val1 > $max )	{
					$cval->SetErr('number exceeds maximum allowed');
					return -1;
				}
			}
			return 1;
		} else {
			$cval->SetErr('invalid number format');
			return -1;
		}
	}

	//	function validateIntegrity($tids, &$data)
	//	{
	//	}

	private function validate($tids, &$data, &$attr)
	{
		if ( is_array( $tids ) ) {
			$isValid = TRUE;
			foreach ( $tids as $tid ) {
				$isValid1 = $this->check($tid, $data, $attr);
				$isValid = $isValid && $isValid1;
			}
		} else {
			$isValid = $this->check($tids, $data, $attr);
		}
		return $isValid;
	}

	private function check($tid, &$data, &$attr)
	{
		switch( $tid ) {
			case 'A_EXT': return $this->chkExtApp($data, $attr);
			case 'A_SCRIPT': return $this->chkScriptHandler($data, $attr);

			case 'L_GENERAL':
			case 'L_GENERAL1':
			case 'ADMIN_L_GENERAL':
			case 'ADMIN_L_GENERAL1':
			case 'VH_TOP':
			case 'VH_REALM': return $this->chkDups( $data, $attr['names'], 'name');
			case 'VH_ERRPG': return $this->chkDups( $data, $attr['names'], 'errCode');
			case 'VH_CTXG':
			case 'VH_CTXJ':
			case 'VH_CTXS':
			case 'VH_CTXF':
			case 'VH_CTXC':
			case 'VH_CTXR':
			case 'VH_CTXRL': return $this->chkDups( $data, $attr['names'], 'uri');
		}
		return TRUE;
	}

	private function chkDups(&$data, &$checkList, $key)
	{
		if ( in_array( $data[$key]->GetVal(), $checkList ) ) {
			$data[$key]->SetErr( "This $key \"" . $data[$key]->GetVal() . '\" already exists. Please enter a different one.');
			return FALSE;
		}
		return TRUE;
	}

	private function chkExtApp(&$data, &$attr)
	{
		$isValid = TRUE;
		if ( $data['autoStart']->GetVal() ) {
			if ( !$data['path']->HasVal() ) {
				$data['path']->SetErr('must provide path when autoStart is enabled');
				$isValid = FALSE;
			}
			if ( !$data['backlog']->HasVal() )	{
				$data['backlog']->SetErr('must enter backlog value when autoStart is enabled');
				$isValid = FALSE;
			}
			if ( !$data['instances']->HasVal() ) {
				$data['instances']->SetErr('must give number of instances when autoStart is enabled');
				$isValid = FALSE;
			}
		}

		if ( isset($attr['names']) && (!$this->chkDups($to, $attr['names'], 'name')) ) {
			$isValid = FALSE;
		}
		return $isValid;
	}

	private function chkScriptHandler(&$to, &$attr)
	{
		$vals = DUtil::splitMultiple( $to['suffix']->GetVal() );
		$isValid = TRUE;
		foreach( $vals as $suffix )	{
			if ( in_array( $suffix, $attr['names'] ) ) {
				$t[] = $suffix;
				$isValid = FALSE;
			}
		}
		if ( !$isValid ) {
			$to['suffix']->SetErr(' Suffix ' . implode(', ', $t) . ' already exists. Please use a different suffix.');
		}
		return $isValid;
	}

}
