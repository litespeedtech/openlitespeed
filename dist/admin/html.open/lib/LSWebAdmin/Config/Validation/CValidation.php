<?php

namespace LSWebAdmin\Config\Validation;

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\Auth\FileThrottleStore;
use LSWebAdmin\Auth\IpThrottle;
use LSWebAdmin\Config\Validation\ExtAppPostTableValidationRule;
use LSWebAdmin\Config\Validation\ListenerPostTableValidationRule;
use LSWebAdmin\Config\Validation\PasswordPostTableValidationRule;
use LSWebAdmin\Config\Validation\ThrottlePostTableValidationRule;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\DAttr;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\UI\DTbl;
use LSWebAdmin\Util\PathTool;
use LSWebAdmin\Config\CNode;

class CValidation
{
    const BLOCK_CMD_SESSION_KEY = 'BLOCK_CMD';

	protected $_request;
	protected $_go_flag;

	public function __construct()
	{

	}

	public function ValidateRequest($request)
	{
		$this->_request = $request;
		$this->_go_flag = 1;

		$tbl = $request->GetTable();
		$tid = $request->GetTid();
		$updatedViewName = null;
		$hasUpdatedViewName = false;

		$extracted = new CNode(CNode::K_EXTRACTED, '', CNode::T_KB);
		$attrs = $tbl->Get(DTbl::FLD_DATTRS);

		foreach ($attrs as $attr) {

			if ($attr->bypassSavePost()) {
				continue;
			}

			$needCheck = $attr->extractPost($extracted, $request->GetInputSource());

			if ($needCheck) {
				if ($attr->_type == 'sel1' || $attr->_type == 'sel2') {
					if ($this->_request->GetAct() == 'c') {
						$needCheck = false; // for changed top category
					} else {
						$attr->SetDerivedSelOptions($request->ResolveDerivedSelOptions($attr->_minVal, $extracted));
					}
				}
				$dlayer = $extracted->GetChildren($attr->GetKey());
				if ($needCheck) {
					$this->validateAttr($attr, $dlayer);
				}
				if (($tid == 'V_TOPD' || $tid == 'V_BASE') && $attr->_type == 'vhname') {
					$updatedViewName = ($dlayer == null) ? null : $dlayer->Get(CNode::FLD_VAL);
					$hasUpdatedViewName = true;
				}
			}
		}

		$res = $this->validatePostTbl($tbl, $extracted);
		$this->setValid($res);

		// if 0 , make it always point to curr page

		if ($this->_go_flag <= 0) {
			$extracted->SetErr('Input error detected. Please resolve the error(s). ');
		}

		$this->_request = null;
		return new ConfigValidationResult($extracted, $updatedViewName, $hasUpdatedViewName);
	}

	protected function setValid($res)
	{
		if ($this->_go_flag != -1) {
			if ($res == -1) {
				$this->_go_flag = -1;
			} elseif ($res == 0 && $this->_go_flag == 1) {
				$this->_go_flag = 0;
			}
		}
		if ($res == 2) {
			$this->_go_flag = 2;
		}
	}

	protected function validatePostTbl($tbl, $extracted)
	{
		$tid = $tbl->Get(DTbl::FLD_ID);

		if (($index = $tbl->Get(DTbl::FLD_INDEX)) != null) {
			$keynode = $extracted->GetChildren($index);
			if ($keynode != null) {
				$holderval = $keynode->Get(CNode::FLD_VAL);
				$extracted->SetVal($holderval);

					if ($holderval != $this->_request->GetCurrentRef()) {
						// check conflict
						$ref = $this->_request->GetParentRef();
						$location = $this->_request->GetTableLocation();
						if (is_string($location) && $location !== '' && $location[0] == '*') { // allow multiple
							$confdata = $this->_request->GetConfData();
							$existingkeys = $confdata->GetChildrenValues($location, $ref);

						if (in_array($holderval, $existingkeys)) {
							$keynode->SetErr('This value has been used! Please choose a unique one.');
							return -1;
						}
					}
				}
			}
		}

		if (($defaultExtract = $tbl->Get(DTbl::FLD_DEFAULTEXTRACT)) != null) {
			foreach ($defaultExtract as $k => $v) {
				$extracted->AddChild(new CNode($k, $v));
			}
		}

		foreach ($this->getPostTableValidationRules() as $rule) {
			if ($rule->Supports($this->_request, $tbl)) {
				return $rule->Validate($this->_request, $extracted);
			}
		}

		return 1;
	}

	protected function getPostTableValidationRules()
	{
		static $rules = null;
		if ($rules == null) {
			$rules = [
				new ListenerPostTableValidationRule(),
				new PasswordPostTableValidationRule(),
				new ExtAppPostTableValidationRule(),
				new ThrottlePostTableValidationRule(),
			];
		}

		return $rules;
	}

	protected function validateAttr($attr, $dlayer)
	{
		if (is_array($dlayer)) {
			foreach ($dlayer as $node) {
				$res = $this->isValidAttr($attr, $node);
				$this->setValid($res);
			}
		} else {
			$res = $this->isValidAttr($attr, $dlayer);
			$this->setValid($res);
		}
	}

	protected function isValidAttr($attr, $node)
	{
		if ($node == null || $node->HasErr()) {
			return -1;
		}

		if (!$node->HasVal()) {
			if (!$attr->IsFlagOn(DAttr::BM_NOTNULL)) {
				return 1;
			}

			$node->SetErr('value must be set. ');
			return -1;
		}

		$notchk = ['cust', 'domain', 'subnet'];
		if (in_array($attr->_type, $notchk)) {
			return 1;
		}

			$chktype = ['uint', 'name', 'vhname', 'dbname', 'admname', 'sel', 'sel1', 'sel2',
				'bool', 'file', 'filep', 'file0', 'file1', 'filetp', 'filevh', 'path', 'note',
				'uri', 'expuri', 'url', 'httpurl', 'email', 'dir', 'addr', 'wsaddr', 'parse', 'charset'];

		if (!in_array($attr->_type, $chktype)) {
			return 1;
		}

		$type3 = substr($attr->_type, 0, 3);
		if ($type3 == 'sel') {
			// for sel, sel1, sel2
			$funcname = 'chkAttr_sel';
		} elseif ($type3 == 'fil' || $type3 == 'pat') {
			$funcname = 'chkAttr_file';
		} else {
			$funcname = 'chkAttr_' . $attr->_type;
		}

		if ($attr->_multiInd == 1) {
			$vals = preg_split("/, /", $node->Get(CNode::FLD_VAL), -1, PREG_SPLIT_NO_EMPTY);
			$err = [];
			$funcname .= '_val';
			foreach ($vals as $i => $v) {
				$res = $this->$funcname($attr, $v, $err[$i]);
				$this->setValid($res);
			}
			$error = trim(implode(' ', $err));
			if ($error != '')
				$node->SetErr($error);
			return 1;
		} else {
			return $this->$funcname($attr, $node);
		}
	}

	protected function chkAttr_sel($attr, $node)
	{
		$err = '';
		$res = $this->chkAttr_sel_val($attr, $node->Get(CNode::FLD_VAL), $err);
		if ($err != '') {
			$node->SetErr($err);
		}
		return $res;
	}

	protected function chkAttr_sel_val($attr, $val, &$err)
	{
		if (isset($attr->_maxVal) && !array_key_exists($val, $attr->_maxVal)) {
			$err = "invalid value: $val";
			return -1;
		}
		return 1;
	}

	protected function chkAttr_admname($attr, $node)
	{
		$val = preg_replace("/\s+/", ' ', trim((string)$node->Get(CNode::FLD_VAL)));
		$node->SetVal($val);

		if ($this->isValidAdminName($val) || $this->isUnchangedLegacyAdminName($val)) {
			return 1;
		}

		$node->SetErr('name can only contain letters, digits, dot, underscore, or hyphen and cannot be longer than 25 characters');
		return -1;
	}

	protected function isValidAdminName($val)
	{
		return (preg_match('/^[A-Za-z0-9._-]{1,25}$/', $val) === 1);
	}

	protected function isUnchangedLegacyAdminName($val)
	{
		if ($this->_request == null
				|| $this->_request->GetView() !== 'admin'
				|| $this->_request->GetTid() !== 'ADM_USR'
				|| $this->_request->GetCurrentRef() !== $val) {
			return false;
		}

		return (strlen($val) > 0
				&& strlen($val) <= 25
				&& escapeshellcmd($val) === $val
				&& !preg_match('/[@:\/]/', $val));
	}

	protected function chkAttr_name($attr, $node)
	{
		$node->SetVal(preg_replace("/\s+/", ' ', $node->Get(CNode::FLD_VAL)));
		$res = $this->chkAttr_name_val($attr, $node->Get(CNode::FLD_VAL), $err);
		if ($err != '') {
			$node->SetErr($err);
		}
		return $res;
	}

	protected function chkAttr_name_val($attr, $val, &$err)
	{
		if (preg_match("/[{}<>&%]/", $val)) {
			$err = 'invalid characters in name';
			return -1;
		}
		if (strlen($val) > 100) {
			$err = 'name cannot be longer than 100 characters';
			return -1;
		}
		return 1;
	}

	protected function chkAttr_note($attr, $node)
	{
		$m = [];
		if (preg_match("/[{}<]/", $node->Get(CNode::FLD_VAL), $m)) { // avoid <script, also {} for conf format
			$node->SetErr("character $m[0] not allowed");
			return -1;
		}
		return 1;
	}

	protected function chkAttr_dbname($attr, $node)
	{
		return $this->chkAttr_vhname($attr, $node);
	}

	protected function chkAttr_vhname($attr, $node)
	{
		$node->SetVal(preg_replace("/\s+/", ' ', $node->Get(CNode::FLD_VAL)));
		$val = $node->Get(CNode::FLD_VAL);
		if (preg_match("/[,;<>&%=\(\)\"']/", $val)) {
			$node->SetErr('Invalid characters found in name');
			return -1;
		}
		if (strpos($val, ' ') !== false) {
			$node->SetErr('No space allowed in the name');
			return -1;
		}
		if (strlen($val) > 100) {
			$node->SetErr('name can not be longer than 100 characters');
			return -1;
		}
		return 1;
	}

	protected function allow_create($attr, $absname)
	{
		if (strpos($attr->_maxVal, 'c') === false) {
			return false;
		}

		if ($attr->_minVal >= 2 && ( strpos($absname, SERVER_ROOT) === 0 )) {
			return true;
		}
		//other places need to manually create
		return false;
	}

	protected function get_cleaned_abs_path($attr_minVal, &$path, &$err)
	{
		if ($this->get_abs_path($attr_minVal, $path, $err) == 1) {
			$absname = $this->clean_absolute_path($path);
			return $absname;
		}
		return null;
	}

	protected function clean_absolute_path($abspath)
	{
		$absname = PathTool::clean($abspath);
		if (isset($_SERVER['LS_CHROOT'])) {
			$root = $_SERVER['LS_CHROOT'];
			$len = strlen($root);
			if (strncmp($absname, $root, $len) == 0) {
				$absname = substr($absname, $len);
			}
		}
		return $absname;
	}

	protected function test_file(&$absname, &$err, $attr)
	{
		if ($attr->_maxVal == null) {
			return 1; // no permission test
		}

		$absname = $this->clean_absolute_path($absname);

		if ($attr->_type == 'file0') {
			if (!file_exists($absname)) {
				return 1; //allow non-exist
			}
		}
		if ($attr->_type == 'path' || $attr->_type == 'filep' || $attr->_type == 'dir') {
			$type = 'path';
		} else {
			$type = 'file';
		}

		if (($type == 'path' && !is_dir($absname)) || ($type == 'file' && !is_file($absname))) {
			$err = $type . ' ' . htmlspecialchars($absname) . ' does not exist.';
			if ($this->allow_create($attr, $absname)) {
				$err .= ' <button type="submit" name="file_create" value="' . htmlspecialchars($attr->GetKey(), ENT_QUOTES) . '" class="lst-btn lst-btn--secondary lst-btn--xs">CLICK TO CREATE</button>';
			} else {
				$err .= ' Please create manually.';
			}

			return -1;
		}
		if ((strpos($attr->_maxVal, 'r') !== false) && !is_readable($absname)) {
			$err = $type . ' ' . htmlspecialchars($absname) . ' is not readable';
			return -1;
		}
		if ((strpos($attr->_maxVal, 'w') !== false) && !is_writable($absname)) {
			$err = $type . ' ' . htmlspecialchars($absname) . ' is not writable';
			return -1;
		}

		return 1;
	}

	protected function chkAttr_file($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		$err = '';
		$res = $this->chkAttr_file_val($attr, $val, $err);
		$node->SetVal($val);
		if ($err != '') {
			$node->SetErr($err);
		}
		return $res;
	}

	protected function chkAttr_dir($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		$err = '';

		if (substr($val, -1) == '*') {
			$res = $this->chkAttr_file_val($attr, substr($val, 0, -1), $err);
		} else {
			$res = $this->chkAttr_file_val($attr, $val, $err);
		}
		$node->SetVal($val);
		if ($err != '') {
			$node->SetErr($err);
		}
		return $res;
	}

	protected function isNotAllowedPath($path)
	{
		$blocked = '/admin/html/';
		if (strpos($path, $blocked) !== false) {
			return true;
		}
		return false;
	}

	protected function isNotAllowedExtension($path)
	{
		$ext = substr($path, -4);
		$notallowed = ['.php', '.cgi', '.pl', '.shtml'];
		foreach ($notallowed as $test) {
			if (strcasecmp($ext, $test) == 0) {
				return true;
			}
		}
		return false;
	}

    protected function check_cmd_invalid_str($cmd)
    {
        if (strlen($cmd) > 256) {
            return 'command too long (> 256 chars)';
        }

        // strip allowed vars first
        $allowed = ['$SERVER_ROOT', '$VH_ROOT', '$VH_NAME', '$DOC_ROOT'];
        $cmd = str_ireplace($allowed, 'a', $cmd);

        // Excessive escaping sequences are almost always evasion
        $escape_cnt = substr_count($cmd, '\\x') + substr_count($cmd, '\\X') + substr_count($cmd, '\\u') + substr_count($cmd, '\\U');
        if ($escape_cnt > 2) {
            return 'suspicious escape sequences detected';
        }

        // Restrict to printable ASCII + safe path characters
        if (!preg_match('/^[\x20-\x7E\/a-zA-Z0-9_.\-+=:,"\'()]*$/', $cmd)) {
            return 'contains non-printable or unusual characters';
        }

        $block_patterns = [
            '/\b(perl|python[23]?|ruby|node|php|bash|sh|dash|zsh|ksh|tclsh|csh|tcsh)\b.*-(?:e|c|r)\b/i' => 'code execution wrapper detected',
            '/\b(curl|wget|fetch|busybox\s+(wget|curl)|aria2c|ftp|lftp|nc|ncat|netcat|socat|telnet)\b/i' => 'dangerous network/download tool',
            '/[;&|`$(){}\x60]/' => 'shell metacharacters / chaining',
            '/https?:\/\/|ftp:\/\//i' => 'suspicious network protocol',
            '/\/dev\/(tcp|udp)/i' => '/dev/(tcp|udp) redirection detected',
            '/:\s*(444[0-9]|555[0-9]|1337|900[0-9]|8080)\b/' => 'suspicious port number',
        ];

        foreach ($block_patterns as $pattern => $msg) {
            if (preg_match($pattern, $cmd)) {
                return $msg;
            }
        }

        if (preg_match('/\\\\[xXuU][0-9a-fA-F]{2}|%[0-9a-fA-F]{2}|[A-Za-z0-9+\/=]{40,}/i', $cmd)) {
            return 'encoded/obfuscated content detected';
        }

        return '';
    }

	public function chkAttr_file_val($attr, $val, &$err)
	{
		// apply to all
		if ($this->isNotAllowedPath($val)) {
			$err = 'Directory not allowed';
			return -1;
		}
		if ($attr->_type == 'file0' && $this->isNotAllowedExtension($val)) {
			$err = 'File extension not allowed';
			return -1;
		}

		clearstatcache();
		$err = null;

		if ($attr->_type == 'filep') {
			$path = substr($val, 0, strrpos($val, '/'));
		} else {
			$path = $val;
			if ($attr->_type == 'file1') { // file1 is command
				$err = $this->check_cmd_invalid_str($path);
				if ($err) {
                    $this->mail_blockcmd(json_encode($path, JSON_UNESCAPED_SLASHES), $err);
					return -1;
				}
				$pos = strpos($val, ' ');
				if ($pos > 0) {
					$path = substr($val, 0, $pos); // check first part is valid path
				}
			}
		}

		$res = $this->chk_file1($attr, $path, $err);
        $productFilePolicyRes = $this->validateManagedConfigFilePolicy($attr, $path, $err);
        if ($productFilePolicyRes != 1) {
            $res = $productFilePolicyRes;
        }

		if ($res == -1 && isset($_POST['file_create']) && $_POST['file_create'] == $attr->GetKey() && $this->allow_create($attr, $path)) {
			if (PathTool::createFile($path, $err, $attr->GetKey())) {
				$err = "$path has been created successfully.";
			}
			$res = 0; // make it always point to curr page
		}

		return $res;
	}

    protected function validateManagedConfigFilePolicy($attr, $path, &$err)
    {
        return 1;
    }

    protected function mail_blockcmd($cmd, $err)
    {
        $ip = isset($_SERVER['REMOTE_ADDR']) ? $_SERVER['REMOTE_ADDR'] : 'unknown';
        $authUser = CAuthorizer::getUserId();
        $serverIp = isset($_SERVER['SERVER_ADDR']) ? $_SERVER['SERVER_ADDR'] : 'unknown';

        error_log("[WebAdmin Console] Blocked Suspicious Command Attempt - username:$authUser ip:$ip err:$err cmd:$cmd\n");

        // Record in abuse throttle bucket — immediate block on first offense.
        if ($ip !== '' && $ip !== 'unknown') {
            $abuseThrottle = new IpThrottle(new FileThrottleStore());
            $abuseThrottle->blockImmediately($ip, 'abuse', 3600);
        }

        $emails = Service::ServiceData(SInfo::DATA_ADMIN_EMAIL);
        if ($emails != null) {

            $repl = [
                '%%host_ip%%' => $serverIp,
                '%%date%%' => gmdate('d-M-Y H:i:s \U\T\C'),
                '%%authUser%%' => $authUser,
                '%%ip%%' => $ip,
                '%%cmd%%' => $cmd,
                '%%reason%%' => $err,
            ];

            $subject = DMsg::UIStr('mail_blockcmd');
            $contents = DMsg::UIStr('mail_blockcmd_c', $repl);

            mail($emails, $subject, $contents);
        }

        if (isset($_SESSION[self::BLOCK_CMD_SESSION_KEY])) {
            $n = $_SESSION[self::BLOCK_CMD_SESSION_KEY] + 1;
            if ($n > 5) {
                header('location:/login.php?failed=1');
            }
            $_SESSION[self::BLOCK_CMD_SESSION_KEY] = $n;
        } else {
            $_SESSION[self::BLOCK_CMD_SESSION_KEY] = 1;
        }
    }

    protected function get_abs_path($attr_minVal, &$path, &$err)
	{
		if (!strlen($path)) {
			$err = "Invalid Path.";
			return -1;
		}

		$s = substr($path, 0, 1);

		if (strpos($path, '$VH_NAME') !== false) {
			$viewName = ($this->_request == null) ? null : $this->_request->GetViewName();
			if ($viewName == null) {
				$err = 'Fail to find $VH_NAME';
				return -1;
			}
			$path = str_replace('$VH_NAME', $viewName, $path);
		}

		if ($s == '/') {
			return 1;
		}

		if ($attr_minVal == 1) {
			$err = 'only accept absolute path. ';
			return -1;
		} elseif ($attr_minVal == 2) {
			if (strncasecmp('$SERVER_ROOT', $path, 12) == 0) {
				$path = SERVER_ROOT . substr($path, 13);
			} elseif ($s == '$') {
				$err = 'only accept absolute path or path relative to $SERVER_ROOT: ' . $path;
				return -1;
			} else {
				$path = SERVER_ROOT . $path; // treat as relative to SERVER_ROOT
			}
		} elseif ($attr_minVal == 3) {
			if (strncasecmp('$SERVER_ROOT', $path, 12) == 0) {
				$path = SERVER_ROOT . substr($path, 13);
			} elseif (strncasecmp('$VH_ROOT', $path, 8) == 0) {
				$vhroot = ($this->_request == null) ? null : $this->_request->GetVHRoot();
				if ($vhroot == null) {
					$err = 'Fail to find $VH_ROOT';
					return -1;
				}
				$path = $vhroot . substr($path, 9);
			} elseif ($s == '$') {
				$err = 'only accept absolute path or path relative to $SERVER_ROOT or $VH_ROOT: ' . $path;
				return -1;
			} else {
				$path = SERVER_ROOT . $path; // treat as relative to SERVER_ROOT
			}
		}

		return 1;
	}

	protected function chk_file1($attr, &$path, &$err)
	{
		$res = $this->get_abs_path($attr->_minVal, $path, $err);
		if ($res == 1) {
			return $this->test_file($path, $err, $attr);
		}
		return $res;
	}

	protected function chkAttr_uri($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		if ($val[0] != '/') {
			$node->SetErr('URI must start with "/"');
			return -1;
		}
		return 1;
	}

	protected function chkAttr_expuri($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		if (substr($val, 0, 1) == '/' || strncmp($val, 'exp:', 4) == 0) {
			return 1;
		}

		$node->SetErr('URI must start with "/" or "exp:"');
		return -1;
	}

	protected function chkAttr_url($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		if (( substr($val, 0, 1) != '/' ) && ( strncmp($val, 'http://', 7) != 0 ) && ( strncmp($val, 'https://', 8) != 0 )) {
			$node->SetErr('URL must start with "/" or "http(s)://"');
			return -1;
		}
		return 1;
	}

	protected function chkAttr_httpurl($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		if (strncmp($val, 'http://', 7) != 0) {
			$node->SetErr('Http URL must start with "http://"');
			return -1;
		}
		return 1;
	}

	protected function chkAttr_email($attr, $node)
	{
		$err = '';
		$res = $this->chkAttr_email_val($attr, $node->Get(CNode::FLD_VAL), $err);
		if ($err != '') {
			$node->SetErr($err);
		}
		return $res;
	}

	protected function chkAttr_email_val($attr, $val, &$err)
	{
		if (preg_match("/^[[:alnum:]._-]+@.+/", $val)) {
			return 1;
		}

		$err = 'invalid email format: ' . $val;
		return -1;
	}

	protected function chkAttr_addr($attr, $node)
	{
		$v = $node->Get(CNode::FLD_VAL);
		if (preg_match("/^([[:alnum:]._-]+|\[[[:xdigit:]:]+\]):(\d)+$/", $v)) {
			return 1;
		}
		if ($this->isUdsAddr($v)) {
			return 1;
		}

		$node->SetErr('invalid address: correct syntax is "IPV4|IPV6_address:port" or UDS://path or unix:path');
		return -1;
	}

	protected function isUdsAddr($v)
	{
		// check UDS:// unix:
		$m = [];
		if (preg_match('/^(UDS:\/\/|unix:)(.+)$/i', $v, $m)) {
			$v = $m[2];
			$supportedvar = ['$SERVER_ROOT', '$VH_NAME', '$VH_ROOT', '$DOC_ROOT'];
			$v = str_replace($supportedvar, 'VAR', $v);
			if (preg_match("/^[a-z0-9\-_\/\.]+$/i", $v)) {
				return 1;
			}
		}
		return 0;
	}

	protected function chkAttr_wsaddr($attr, $node)
	{
		$v = $node->Get(CNode::FLD_VAL);
		if (preg_match("/^((http|https):\/\/)?([[:alnum:]._-]+|\[[[:xdigit:]:]+\])(:\d+)?$/", $v)) {
			return 1;
		}
		if ($this->isUdsAddr($v)) {
			return 1;
		}

		$node->SetErr('invalid address: correct syntax is "[http|https://]IPV4|IPV6_address[:port]" or Unix Domain Socket address "UDS://path or unix:path".');
		return -1;
	}

	protected function chkAttr_bool($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		if ($val === '1' || $val === '0') {
			return 1;
		}
		$node->SetErr('invalid value');
		return -1;
	}

	protected function chkAttr_charset($attr, $node)
	{
		$val = trim($node->Get(CNode::FLD_VAL));
		$lower = strtolower($val);

		if ($lower === 'on' || $lower === 'off') {
			$node->SetVal($lower);
			return 1;
		}

		if (preg_match('/^[A-Za-z0-9][A-Za-z0-9._-]*$/', $val)) {
			$node->SetVal($val);
			return 1;
		}

		$node->SetErr('invalid charset value');
		return -1;
	}

	protected function chkAttr_parse($attr, $node)
	{
		$err = '';
		$res = $this->chkAttr_parse_val($attr, $node->Get(CNode::FLD_VAL), $err);
		if ($err != '') {
			$node->SetErr($err);
		}
		return $res;
	}

	protected function chkAttr_parse_val($attr, $val, &$err)
	{
		if (preg_match($attr->_minVal, $val)) {
			return 1;
		}
		if ($attr->_maxVal) { // has parse_help
			$err = "invalid format \"$val\". Syntax is {$attr->_minVal} - {$attr->_maxVal}";
		} else {
			// when no parse_help, do not show syntax, e.g. used for not allowed value.
			$err = "invalid value \"$val\".";
		}
		return -1;
	}

	protected function getKNum($strNum)
	{
		$tag = strtoupper(substr($strNum, -1));
		switch ($tag) {
			case 'K': $multi = 1024;
				break;
			case 'M': $multi = 1048576;
				break;
			case 'G': $multi = 1073741824;
				break;
			default: return intval($strNum);
		}

		return (intval(substr($strNum, 0, -1)) * $multi);
	}

	protected function chkAttr_uint($attr, $node)
	{
		$val = $node->Get(CNode::FLD_VAL);
		if (preg_match("/^(-?\d+)([KkMmGg]?)$/", $val)) {
			$val1 = $this->getKNum($val);
			if (isset($attr->_minVal)) {
				$min = $this->getKNum($attr->_minVal);
				if ($val1 < $min) {
					$node->SetErr('number is less than the minimum required');
					return -1;
				}
			}
			if (isset($attr->_maxVal)) {
				$max = $this->getKNum($attr->_maxVal);
				if ($val1 > $max) {
					$node->SetErr('number exceeds maximum allowed');
					return -1;
				}
			}
			return 1;
		}

		$node->SetErr('invalid number format');
		return -1;
	}

}
