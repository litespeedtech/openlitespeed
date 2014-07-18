<?php

require_once('blowfish.php');

class CAuthorizer
{
	private $_id;
	private $_id_field;
	private $_pass;
	private $_pass_field;

	private static $_instance = NULL;

	// prevent an object from being constructed
	private function __construct()
	{
		$label = str_replace('/', '_', SERVER_ROOT);
		$this->_id_field = "{$label}_uid";
		$this->_pass_field = "{$label}_pass";

		global $_SESSION;

		session_name("{$label}WEBUI"); // to prevent conflicts with other app sessions
		session_start();

		if(!array_key_exists('changed',$_SESSION)) {
			$_SESSION['changed'] = FALSE;
		}


		if(!array_key_exists('valid',$_SESSION)) {
			$_SESSION['valid'] = FALSE;
		}

		if(!array_key_exists('token',$_SESSION)) {
			$_SESSION['token'] = microtime();
		}

		if($_SESSION['valid']) {

			if (array_key_exists('lastaccess',$_SESSION)) {

				if($_SESSION['timeout'] > 0 && time() - $_SESSION['lastaccess'] > $_SESSION['timeout'] ) {
					$this->clear();
					header("location:/login.php?timedout=1");
					die();
				}

				$this->id = GUIBase::GrabGoodInput('cookie',$this->_id_field);
				$this->pass = GUIBase::GrabInput('cookie',$this->_pass_field);

			}
			$this->updateAccessTime();
		}
	}

	public static function singleton() {

		if (!isset(self::$_instance)) {
			$c = __CLASS__;
			self::$_instance = new $c;
		}

		return self::$_instance;
	}

	public static function Authorize()
	{
		$auth = CAuthorizer::singleton();
		if (!$auth->IsValid()) {
			$auth->clear();
			header('location:/login.php');
			die();
		}
	}

	public function IsValid()
	{
		global $_SESSION;
		return !( ($_SESSION['valid'] !== TRUE)
				|| (isset($_SERVER['HTTP_REFERER'])
						&& strpos($_SERVER['HTTP_REFERER'], $_SERVER['HTTP_HOST']) === FALSE));
	}

	public static function GetToken()
	{
		global $_SESSION;
		return $_SESSION['token'];
	}

	public static function SetTimeout($timeout) {
		global $_SESSION;
		$_SESSION['timeout'] = (int) $timeout;
	}

	public static function HasSetTimeout() {
		global $_SESSION;
		return (isset($_SESSION['timeout']) && $_SESSION['timeout'] >= 60);
	}

	public function GetCmdHeader()
	{
		global $_SESSION;
		if (isset($_SESSION['secret']) && is_array($_SESSION['secret'])) {
			$uid = PMA_blowfish_decrypt( $this->id, $_SESSION['secret'][0]);
			$password = PMA_blowfish_decrypt( $this->pass, $_SESSION['secret'][1]);
			return "auth:$uid:$password\n";
		}
		else
			return '';
	}

	public function GenKeyPair()
	{
		$keyLength = 512;
		$cc = ConfControl::singleton();
		$keyfile = $cc->GetConfFilePath('admin', 'key');
		$mykeys = NULL;
		if (file_exists($keyfile)) {
			$str = file_get_contents($keyfile);
			if ($str != '') {
				$mykeys = unserialize($str);
			}
		}
		if ($mykeys == NULL) {
			$jCryption = new jCryption();
			$keys = $jCryption->generateKeypair($keyLength);
			$e_hex = $jCryption->dec2string($keys['e'],16);
			$n_hex = $jCryption->dec2string($keys['n'],16);
			$mykeys = array( 'e_hex' => $e_hex, 'n_hex' => $n_hex, 'd_int' => $keys['d'], 'n_int' => $keys['n']);
			$serialized_str = serialize($mykeys);
			file_put_contents($keyfile, $serialized_str);
			chmod($keyfile, 0600);
		}
		global $_SESSION;
		$_SESSION['d_int'] = $mykeys['d_int'];
		$_SESSION['n_int'] = $mykeys['n_int'];

		return '{"e":"'.$mykeys['e_hex'].'","n":"'.$mykeys['n_hex'].'","maxdigits":"'.intval($keyLength*2/16+3).'"}';
	}

	public function ShowLogin($is_https, &$msg)
	{
		$timedout = GUIBase::GrabInput('get','timedout','int');
		$logoff = GUIBase::GrabInput('get','logoff','int');
		$msg = '';

		if($timedout == 1 || $logoff  == 1) {
			$this->clear();

			if($timedout == 1) {
				$msg = 'Your session has timed out.';
			}
			else {
				$msg = 'You have logged off.';
			}
		}
		else if($this->IsValid()) {
			return FALSE;
		}

		$userid = NULL;
		$pass = NULL;

		if ( isset($_POST['jCryption']) )
		{
			global $_SESSION;
			$jCryption = new jCryption();
			$var = $jCryption->decrypt($_POST['jCryption'], $_SESSION['d_int'], $_SESSION['n_int']);
			unset($_SESSION['d_int']);
			unset($_SESSION['n_int']);
			parse_str($var,$result);
			$userid = $result['userid'];
			$pass = $result['pass'];
		}
		else if ($is_https && isset($_POST['userid'])) {
			$userid = GUIBase::GrabGoodInput('POST','userid');
			$pass = GUIBase::GrabInput('POST','pass');
		}

		if ($userid != NULL) {
			if ( $this->authenticate($userid, $pass) === TRUE )
				return FALSE;
			else
				$msg = 'Invalid credentials.';
		}
		return TRUE;

	}


	private function updateAccessTime($secret=NULL) {
		global $_SESSION;
		$_SESSION['lastaccess'] = time();
		if (isset($secret)) {
			$_SESSION['valid'] = TRUE;;
			$_SESSION['secret'] = $secret;
		}
	}

	private function clear()
	{
		session_destroy();
		session_unset();
		$outdated = time() - 3600*24*30;
		setcookie($this->_id_field, "a", $outdated, "/");
		setcookie($this->_pass_field, "b", $outdated, "/");
		setcookie(session_name(), "b", $outdated, "/");
	}


	private function authenticate($authUser, $authPass)
	{
		$auth = FALSE;

		if (strlen($authUser) && strlen($authPass) )
		{
			$filename = SERVER_ROOT . 'admin/conf/htpasswd';

			$fd = fopen( $filename, 'r');
			if ( !$fd) {
				return FALSE;
			}

			$all = trim(fread($fd, filesize($filename)));
			fclose($fd);

			$lines = explode("\n", $all);
			foreach( $lines as $line )
			{
				list($user, $pass) = explode(':', $line);
				if ( $user == $authUser )
				{
				    if ( $pass[0] != '$' )
						$salt = substr($pass, 0, 2 );
				    else
						$salt = substr($pass, 0, 12 );
				    $encypt = crypt($authPass, $salt);
				    if ( $pass == $encypt )
				    {
						$auth = TRUE;
						break;
				    }
				}
			}
		}

		if ($auth) {
			$temp=gettimeofday();
			$start=(int)$temp['usec'];
			$secretKey0 = mt_rand(). $start . mt_rand();
			$secretKey1 = mt_rand(). mt_rand() . $start;

			setcookie($this->_id_field, PMA_blowfish_encrypt($authUser, $secretKey0), 0, "/");
			setcookie($this->_pass_field, PMA_blowfish_encrypt($authPass, $secretKey1), 0, "/");

			$this->updateAccessTime(array($secretKey0, $secretKey1));
		}
		else {
			$this->emailFailedLogin($authUser);
		}

		return $auth;
	}

	private function emailFailedLogin($authUser)
	{
		$subject = 'LiteSpeed Web Admin Console Failed Login Attempt';
		$ip = $_SERVER['REMOTE_ADDR'];
		$url = $_SERVER['SCRIPT_URI'];

		error_log("$subject - username: $authUser ip: $ip url: $url\n");

		$emails = ConfControl::GetAdminEmails();
		if ($emails != NULL) {
			$hostname = gethostbyaddr($ip);
			$date = date("F j, Y, g:i a");
			$contents = "A recent login attempt to LiteSpeed web admin console failed. Details of the attempt are below.\n
	Date/Time: $date
	Username: $authUser
	IP Address: $ip
	Hostname: $hostname
	URL: $url

If you do not recognize the IP address, please follow below recommended ways to secure your admin console:

	1. set access allowed list to limit certain IPs that can access under WebAdmin WebConsole->General->AccessControl;
	2. change the listener port from default value 7080;
	3. do not use simple password;
	4. use https for admin console.
			";
			$result = mail($emails, $subject, $contents);
		}
	}

}
