<?php

class CValidation
{
	protected $_info;

	public function __construct() {
		$this->_info = NULL;
	}
	
	public function ExtractPost($tbl, &$d, $disp)
	{
		$this->_info = $disp->_info;
		$goFlag = 1 ;
		$index = array_keys($tbl->_dattrs);
		foreach ( $index as $i ) {
			$attr = $tbl->_dattrs[$i];

			if ( $attr == NULL || $attr->bypassSavePost()) 
				continue;
				
			$d[$attr->_key] = $attr->extractPost();
			$needCheck = TRUE;
			if ( $attr->_type == 'sel1' || $attr->_type == 'sel2' )	{
				if ( $disp->_act == 'c' ) {
					$needCheck = FALSE;
				} 
				else {
					$attr->populate_sel1_options($this->_info, $d);
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

	protected function checkListener(&$listener)
	{
		if ( $listener['secure']->GetVal() == '0' ) {
			if ( isset($listener['certFile']) && !$listener['certFile']->HasVal() ) {
				$listener['certFile']->SetErr(NULL);
			}
			if ( isset($listener['keyFile']) && !$listener['keyFile']->HasVal() ) {
				$listener['keyFile']->SetErr(NULL);
			}
		} else {
			$tids = array('L_SSL_CERT');
			$this->validateElement($tids, $listener);
		}
	}

	protected function validateElement($tids, &$data)
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

	protected function setValid(&$valid, $res)
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

	protected function validatePostTbl($tbl, &$d)
	{
		$isValid = 1;
		if ( $tbl->_holderIndex != NULL && isset($d[$tbl->_holderIndex])) {
			$newref = $d[$tbl->_holderIndex]->GetVal();
			$oldref = NULL;

			if(isset($this->_info['holderIndex_cur'])) {
				$oldref = $this->_info['holderIndex_cur'];
			}
			//echo "oldref = $oldref newref = $newref \n";
			if ( $oldref == NULL || $newref != $oldref ) {
				if (isset($this->_info['holderIndex']) && $this->_info['holderIndex'] != NULL
				&& in_array($newref, $this->_info['holderIndex']) ) {
					$d[$tbl->_holderIndex]->SetErr('This value has been used! Please choose a unique one.');
					$isValid = -1;
				}
			}
		}
		
		$checkedTids = array('VH_TOP_D','VH_BASE','VH_UDB',	'ADMIN_USR', 'ADMIN_USR_NEW', 
		'L_GENERAL', 'L_GENERAL1', 'ADMIN_L_GENERAL', 'ADMIN_L_GENERAL1', 'L_SSL', 'L_CERT', 'L_SSL_CERT', 
		'TP', 'TP1');

		if ( in_array($tbl->_id, $checkedTids) ) {
			switch ($tbl->_id) {
				case 'TP':
				case 'TP1':
					$isValid = $this->chkPostTbl_TP($d);
					break;
				case 'VH_BASE':
				case 'VH_TOP_D':
					$isValid = $this->chkPostTbl_VH_BASE($d);
					break;
				case 'VH_UDB':
					$isValid = $this->chkPostTbl_VH_UDB($d);
					break;
				case 'ADMIN_USR':
					$isValid = $this->chkPostTbl_ADMIN_USR($d);
					break;
				case 'ADMIN_USR_NEW':
					$isValid = $this->chkPostTbl_ADMIN_USR_NEW($d);
					break;
				case 'L_GENERAL':
				case 'L_GENERAL1':
				case 'ADMIN_L_GENERAL':
				case 'ADMIN_L_GENERAL1':
					$isValid = $this->chkPostTbl_L_GENERAL($d);
					break;
				case 'L_SSL':
					$isValid = $this->chkPostTbl_L_SSL($d);
					break;
				case 'L_CERT':
				case 'L_SSL_CERT':
					$isValid = $this->chkPostTbl_L_SSL_CERT($d);
					break;
			}
		}
		
		return $isValid;
	}
	
	
	protected function chkPostTbl_TP(&$d)
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
	
	protected function chkPostTbl_VH_BASE(&$d)
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
	
	protected function chkPostTbl_VH_UDB(&$d)
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

	protected function encryptPass($val)
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

	protected function chkPostTbl_ADMIN_USR(&$d)
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

	protected function chkPostTbl_ADMIN_USR_NEW(&$d)
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

	protected function chkPostTbl_L_GENERAL(&$d)
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
	
	protected function isCurrentListenerSecure()
	{
		$confCenter = ConfCenter::singleton();
		$listenerName = trim($confCenter->GetDispInfo()->_name);
		$l = $confCenter->_serv->_data['listeners'][$listenerName];
		return ($l['secure']->GetVal() == 1);
	}
	
	protected function chkPostTbl_L_SSL(&$d)
	{
		$isValid = 1;
		if ($this->isCurrentListenerSecure()) {
			$err = 'Value must be set for secured listener';
			if (!$d['sslProtocol']->HasVal()) {
				$d['sslProtocol']->SetErr($err);
				$isValid = -1;
			}
			if (!$d['ciphers']->HasVal()) {
				$d['ciphers']->SetErr($err);
				$isValid = -1;
			}
		}
		
		return $isValid;
	}

	protected function chkPostTbl_L_SSL_CERT(&$d)
	{
		$isValid = 1;
		if ($this->isCurrentListenerSecure()) {
			$err = 'Value must be set for secured listener';
			if (!$d['keyFile']->HasVal()) {
				$d['keyFile']->SetErr($err);
				$isValid = -1;
			}
			if (!$d['certFile']->HasVal()) {
				$d['certFile']->SetErr($err);
				$isValid = -1;
			}
		}
		
		return $isValid;
	}
	
	protected function validateTblAttr($tblDef, $tbl, &$data)
	{
		$valid = 1;
		if ( $tbl->_subTbls != NULL ) {
			$tid = DUtil::getSubTid($tbl->_subTbls, $data);
			if ( $tid == NULL ) {
				return;
			}
			$tbl1 = $tblDef->GetTblDef($tid);
		} else {
			$tbl1 = $tbl;
		}

		$index = array_keys($tbl1->_dattrs);
		foreach ( $index as $i ) {
			$attr = $tbl1->_dattrs[$i];

			if ( $attr->_type == 'sel1' || $attr->_type == 'sel2' ) {
				$attr->populate_sel1_options($this->_info, $data);
			}

			$res = $this->validateAttr($attr, $data[$attr->_key]);
			$this->setValid($valid, $res);
		}
		return $valid;
	}

	protected function validateAttr($attr, &$cvals)
	{
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

	protected function isValidAttr($attr, $cval)
	{
		if ($cval == NULL || $cval->HasErr())
			return -1;
			
		if ( !$cval->HasVal()) {
			if ( $attr->_allowNull ) {
				return 1;
			}
			$cval->SetErr('value must be set');
			return -1;
		}

		if ( $attr->_type == 'cust' ) {
			return 1;
		}
		
		$chktype = array('uint', 'name', 'vhname', 'sel','sel1','sel2',
		'bool','file','filep','file0','file1', 'filetp', 'path',
		'uri','expuri','url', 'httpurl', 'email', 'dir', 'addr', 'parse');
		
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

	protected function chkAttr_sel($attr, $cval)
	{
		$err = '';
		$res = $this->chkAttr_sel_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}

	protected function chkAttr_sel_val($attr, $val, &$err)
	{
		if ( isset( $attr->_maxVal ) 
			&& !array_key_exists($val, $attr->_maxVal) ) {
				$err = "invalid value: $val";
			return -1;
		}
		return 1;
	}
	
	protected function chkAttr_name($attr, $cval)
	{
		$cval->SetVal( preg_replace("/\s+/", ' ', $cval->GetVal()));
		$res = $this->chkAttr_name_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}

	protected function chkAttr_name_val($attr, $val, &$err)
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
	
	protected function chkAttr_vhname($attr, $cval)
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

	protected function allow_create($attr, $absname)
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

	protected function test_file(&$absname, &$err, $attr)
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

	protected function chkAttr_file($attr, $cval)
	{
		$val = $cval->GetVal();
		$err = '';
		$res = $this->chkAttr_file_val($attr, $val, $err);
		$cval->SetVal($val);
		$cval->SetErr($err);
		return $res;
	}
	
	protected function chkAttr_dir($attr, $cval)
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

	protected function chk_file1($attr, &$path, &$err)
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

	protected function chkAttr_uri($attr, $cval)
	{
		$val = $cval->GetVal();
		if ( $val[0] != '/' ) {
			$cval->SetErr('URI must start with "/"');
			return -1;
		}
		return 1;
	}

	protected function chkAttr_expuri($attr, $cval)
	{
		$val = $cval->GetVal();
		if ( $val{0} == '/' || strncmp( $val, 'exp:', 4 ) == 0 ) {
			return 1;
		} else {
			$cval->SetErr('URI must start with "/" or "exp:"');
			return -1;
		}
	}

	protected function chkAttr_url($attr, $cval)
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

	protected function chkAttr_httpurl($attr, $cval)
	{
		$val = $cval->GetVal();
		if (strncmp( $val, 'http://', 7 ) != 0 ) {
			$cval->SetErr('Http URL must start with "http://"');
			return -1;
		}
		return 1;
	}
	
	protected function chkAttr_email($attr, $cval)
	{
		$err = '';
		$res = $this->chkAttr_email_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}
	
	protected function chkAttr_email_val($attr, $val, &$err)
	{
		if ( preg_match("/^[[:alnum:]._-]+@.+/", $val ) ) {
			return 1;
		} else {
			$err = 'invalid email format: ' . $val;
			return -1;
		}
	}
	
	protected function chkAttr_addr($attr, $cval)
	{
		if ( preg_match("/^[[:alnum:]._-]+:(\d)+$/", $cval->GetVal()) ) {
			return 1;
		} elseif ( preg_match("/^UDS:\/\/.+/i", $cval->GetVal()) ) {
			return 1;
		} else {
			$cval->SetErr('invalid address: correct syntax is "IPV4|IPV6_address:port" or UDS://path');
			return -1;
		}
	}

	protected function chkAttr_bool($attr, $cval)
	{
		$val = $cval->GetVal();
		if ( $val === '1' || $val === '0' ) {
			return 1;
		}
		$cval->SetErr('invalid value');
		return -1;
	}

	protected function chkAttr_parse($attr, $cval)
	{
		$err = '';
		$res = $this->chkAttr_parse_val($attr, $cval->GetVal(), $err);
		$cval->SetErr($err);
		return $res;
	}
	
	protected function chkAttr_parse_val($attr, $val, &$err)
	{
		if ( preg_match($attr->_minVal, $val) ) {
			return 1;
		} else {
			$err = "invalid format - $val, syntax is {$attr->_maxVal}";
			return -1;
		}
	}

	protected function getKNum($strNum)
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

	protected function chkAttr_uint($attr, $cval)
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

	protected function chkDups(&$data, &$checkList, $key)
	{
		if ( in_array( $data[$key]->GetVal(), $checkList ) ) {
			$data[$key]->SetErr( "This $key \"" . $data[$key]->GetVal() . '\" already exists. Please enter a different one.');
			return FALSE;
		}
		return TRUE;
	}

	protected function chkExtApp(&$data, &$attr)
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

	protected function chkScriptHandler(&$to, &$attr)
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
