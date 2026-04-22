<?php

namespace LSWebAdmin\Controller;

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\I18n\DMsg;

class AdminCommandClient
{
    const CONNECT_TIMEOUT_SEC = 1;
    const COMMAND_IO_TIMEOUT_SEC = 1;

    private $_lastError = '';

    public function sendCommand($cmd)
    {
        $sock = $this->openAuthenticatedSocket($cmd);
        if (!$sock) {
            return false;
        }

        if (!$this->setSocketTimeout($sock, SO_RCVTIMEO, self::COMMAND_IO_TIMEOUT_SEC, 'cmd ' . trim($cmd) . ' failed to set receive timeout on admin socket')) {
            socket_close($sock);
            return false;
        }

        $buffer = '';
        $res = @socket_recv($sock, $buffer, 1024, 0);
        $recvError = ($res === false) ? socket_last_error($sock) : 0;
        socket_close($sock);

        if ($res === false) {
            $this->setLastError($this->buildSocketErrorMessage(socket_strerror($recvError)));
            return false;
        }

        if ($res <= 0) {
            $this->setLastError(DMsg::UIStr('service_requestfailed'));
            return false;
        }

        if (strncasecmp($buffer, 'OK', 2) != 0) {
            $message = trim($buffer);
            $this->setLastError(($message !== '') ? $message : DMsg::UIStr('service_requestfailed'));
            return false;
        }

        return true;
    }

    public function fetchCommandData($cmd, $timeout = 3)
    {
        $sock = $this->openAuthenticatedSocket($cmd);
        $buffer = '';
        if ($sock) {
            $readTimeout = ((int) $timeout > 0) ? (int) $timeout : self::COMMAND_IO_TIMEOUT_SEC;
            if (!$this->setSocketTimeout($sock, SO_RCVTIMEO, $readTimeout, 'cmd ' . trim($cmd) . ' failed to set receive timeout on admin socket')) {
                socket_close($sock);
                return $buffer;
            }

            $read = [$sock];
            $write = null;
            $except = null;
            $num_changed_sockets = socket_select($read, $write, $except, $readTimeout);
            if ($num_changed_sockets === false) {
                error_log("socket_select failed: " . socket_strerror(socket_last_error()));
            } elseif ($num_changed_sockets > 0) {
                while (($bytes = @socket_recv($sock, $data, 8192, 0)) > 0) {
                    $buffer .= $data;
                }

                if ($bytes === false) {
                    error_log('cmd ' . trim($cmd) . ' failed to read from admin socket: ' . socket_strerror(socket_last_error($sock)));
                }
            }
            socket_close($sock);
        }

        return $buffer;
    }

    public function openAuthenticatedSocket($cmd)
    {
        $this->clearLastError();

        $adminSock = $this->getAdminSocketEnv($cmd);
        if ($adminSock === false) {
            return false;
        }

        $sock = $this->connectSocket($cmd, $adminSock);
        if ($sock === false) {
            return false;
        }

        if (!$this->setSocketTimeout($sock, SO_SNDTIMEO, self::COMMAND_IO_TIMEOUT_SEC, 'cmd ' . trim($cmd) . ' failed to set send timeout on admin socket: ' . $adminSock)) {
            socket_close($sock);
            return false;
        }

        $outBuf = CAuthorizer::singleton()->GetCmdHeader() . $cmd . "\nend of actions";
        if (socket_write($sock, $outBuf) === false) {
            $this->setLastError($this->buildSocketErrorMessage(socket_strerror(socket_last_error($sock))));
            error_log('cmd ' . $cmd . ' failed to write to admin socket: ' . socket_strerror(socket_last_error($sock)) . " $adminSock\n");
            socket_close($sock);
            return false;
        }

        socket_shutdown($sock, 1);
        return $sock;
    }

    public function getLastError()
    {
        return $this->_lastError;
    }

    private function getAdminSocketEnv($cmd)
    {
        if (!isset($_SERVER['LSWS_ADMIN_SOCK']) || $_SERVER['LSWS_ADMIN_SOCK'] === '') {
            $this->setLastError($this->buildSocketErrorMessage('LSWS_ADMIN_SOCK is not set.'));
            error_log('LSWS_ADMIN_SOCK is not set. Please check lsws/admin/tmp/ directory and empty all files there, then restart.');
            return false;
        }

        return $_SERVER['LSWS_ADMIN_SOCK'];
    }

    private function connectSocket($cmd, $adminSock)
    {
        if (strncmp($adminSock, 'uds://', 6) == 0) {
            return $this->connectUnixSocket($cmd, $adminSock);
        }

        return $this->connectInetSocket($cmd, $adminSock);
    }

    private function connectUnixSocket($cmd, $adminSock)
    {
        $sock = socket_create(AF_UNIX, SOCK_STREAM, 0);
        if ($sock === false) {
            $this->setLastError($this->buildSocketErrorMessage(socket_strerror(socket_last_error())));
            error_log('cmd ' . $cmd . ' failed to create AF_UNIX socket: ' . socket_strerror(socket_last_error()) . " $adminSock\n");
            return false;
        }

        $chrootOffset = isset($_SERVER['LS_CHROOT']) ? strlen($_SERVER['LS_CHROOT']) : 0;
        $addr = substr($adminSock, 5 + $chrootOffset);
        if (!$this->connectWithTimeout($sock, $cmd, $adminSock, 'admsock', $addr)) {
            return false;
        }

        return $sock;
    }

    private function connectInetSocket($cmd, $adminSock)
    {
        $addr = explode(":", $adminSock, 2);
        if (count($addr) != 2 || $addr[0] === '' || !ctype_digit($addr[1])) {
            $this->setLastError($this->buildSocketErrorMessage('Invalid LSWS_ADMIN_SOCK format.'));
            error_log('Invalid LSWS_ADMIN_SOCK format for AF_INET socket: ' . $adminSock);
            return false;
        }

        $sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
        if ($sock === false) {
            $this->setLastError($this->buildSocketErrorMessage(socket_strerror(socket_last_error())));
            error_log('cmd ' . $cmd . ' failed to create AF_INET socket: ' . socket_strerror(socket_last_error()) . " $adminSock\n");
            return false;
        }

        if (!$this->connectWithTimeout($sock, $cmd, $adminSock, 'AF_INET', $addr[0], intval($addr[1]))) {
            return false;
        }

        return $sock;
    }

    private function connectWithTimeout($sock, $cmd, $adminSock, $label)
    {
        $connectArgs = array_slice(func_get_args(), 4);
        if (!socket_set_nonblock($sock)) {
            $this->logSocketFailure($cmd, $adminSock, 'failed to enable non-blocking mode for ' . $label, socket_last_error($sock), $sock);
            return false;
        }

        $res = $this->socketConnectArgs($sock, $connectArgs);
        if ($res === false) {
            $errno = socket_last_error($sock);
            if (!$this->isConnectInProgress($errno)) {
                $this->logSocketFailure($cmd, $adminSock, 'failed to connect to server via ' . $label, $errno, $sock);
                return false;
            }
        }

        $read = null;
        $write = [$sock];
        $except = [$sock];
        $selected = socket_select($read, $write, $except, self::CONNECT_TIMEOUT_SEC);
        if ($selected === false) {
            $this->logSocketFailure($cmd, $adminSock, 'socket_select() failed while connecting via ' . $label, socket_last_error(), $sock);
            return false;
        }

        if ($selected === 0) {
            $detail = defined('SOCKET_ETIMEDOUT')
                ? socket_strerror(SOCKET_ETIMEDOUT)
                : 'Connection timed out';
            $this->setLastError($this->buildSocketErrorMessage($detail));
            error_log('cmd ' . trim($cmd) . ' failed to connect to server via ' . $label . '! socket_connect() timed out after ' . self::CONNECT_TIMEOUT_SEC . " second(s): $adminSock\n");
            socket_close($sock);
            return false;
        }

        $errno = socket_get_option($sock, SOL_SOCKET, SO_ERROR);
        if ($errno !== 0) {
            $this->logSocketFailure($cmd, $adminSock, 'failed to connect to server via ' . $label, $errno, $sock);
            return false;
        }

        socket_clear_error($sock);

        if (!socket_set_block($sock)) {
            $this->logSocketFailure($cmd, $adminSock, 'failed to restore blocking mode for ' . $label, socket_last_error($sock), $sock);
            return false;
        }

        return true;
    }

    private function socketConnectArgs($sock, $connectArgs)
    {
        if (count($connectArgs) == 1) {
            return @socket_connect($sock, $connectArgs[0]);
        }

        return @socket_connect($sock, $connectArgs[0], $connectArgs[1]);
    }

    private function isConnectInProgress($errno)
    {
        $pendingErrors = ['SOCKET_EINPROGRESS', 'SOCKET_EALREADY', 'SOCKET_EWOULDBLOCK'];

        foreach ($pendingErrors as $constant) {
            if (defined($constant) && $errno === constant($constant)) {
                return true;
            }
        }

        return false;
    }

    private function logSocketFailure($cmd, $adminSock, $context, $errno, $sock)
    {
        $detail = socket_strerror($errno);
        $this->setLastError($this->buildSocketErrorMessage($detail));
        error_log('cmd ' . trim($cmd) . ' ' . $context . ': ' . $detail . " $adminSock\n");
        socket_close($sock);
    }

    private function setSocketTimeout($sock, $option, $seconds, $context)
    {
        $timeout = [
            'sec' => ((int) $seconds > 0) ? (int) $seconds : 1,
            'usec' => 0,
        ];

        if (!socket_set_option($sock, SOL_SOCKET, $option, $timeout)) {
            $this->setLastError($this->buildSocketErrorMessage(socket_strerror(socket_last_error($sock))));
            error_log($context . ': ' . socket_strerror(socket_last_error($sock)) . "\n");
            return false;
        }

        return true;
    }

    private function buildSocketErrorMessage($detail)
    {
        $message = DMsg::UIStr('service_adminsocket_connectfailed');

        if ($detail !== '') {
            $message .= ': ' . $detail;
        }

        return $message;
    }

    private function setLastError($message)
    {
        $this->_lastError = $message;
    }

    private function clearLastError()
    {
        $this->_lastError = '';
    }
}
