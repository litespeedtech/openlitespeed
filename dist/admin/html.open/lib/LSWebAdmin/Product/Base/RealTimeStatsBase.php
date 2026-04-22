<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\UI\UIBase;

abstract class RealTimeStatsBase
{

    const FLD_BPS_IN = 0;
    const FLD_BPS_OUT = 1;
    const FLD_SSL_BPS_IN = 2;
    const FLD_SSL_BPS_OUT = 3;
    const FLD_PLAINCONN = 4;
    const FLD_IDLECONN = 5;
    const FLD_SSLCONN = 6;
    const FLD_S_REQ_PROCESSING = 7;
    const FLD_S_REQ_PER_SEC = 8;
    const FLD_UPTIME = 9;
    const FLD_MAXCONN = 10;
    const FLD_MAXSSL_CONN = 11;
    const FLD_AVAILCONN = 12;
    const FLD_AVAILSSL = 13;
    const FLD_BLOCKEDIP_COUNT = 14;
    const FLD_S_TOT_REQS = 15;
    const FLD_S_PUB_CACHE_HITS_PER_SEC = 16;
    const FLD_S_TOTAL_PUB_CACHE_HITS = 17;
    const FLD_S_PRIV_CACHE_HITS_PER_SEC = 18;
    const FLD_S_TOTAL_PRIV_CACHE_HITS = 19;
    const FLD_S_STATIC_HITS_PER_SEC = 20;
    const FLD_S_TOTAL_STATIC_HITS = 21;
    const COUNT_FLD_SERVER = 22;
    const FLD_BLOCKEDIP = 22;
    //const FLD_VH_NAME = 0;
    const FLD_VH_EAP_INUSE = 0;
    const FLD_VH_EAP_WAITQUE = 1;
    const FLD_VH_EAP_IDLE = 2;
    const FLD_VH_EAP_REQ_PER_SEC = 3;
    const FLD_VH_REQ_PROCESSING = 4;
    const FLD_VH_REQ_PER_SEC = 5;
    const FLD_VH_TOT_REQS = 6;
    const FLD_VH_EAP_COUNT = 7;
    const FLD_VH_PUB_CACHE_HITS_PER_SEC = 8;
    const FLD_VH_TOTAL_PUB_CACHE_HITS = 9;
    const FLD_VH_PRIV_CACHE_HITS_PER_SEC = 10;
    const FLD_VH_TOTAL_PRIV_CACHE_HITS = 11;
    const FLD_VH_STATIC_HITS_PER_SEC = 12;
    const FLD_VH_TOTAL_STATIC_HITS = 13;
    const COUNT_FLD_VH = 14;
    const FLD_EA_CMAXCONN = 0;
    const FLD_EA_EMAXCONN = 1;
    const FLD_EA_POOL_SIZE = 2;
    const FLD_EA_INUSE_CONN = 3;
    const FLD_EA_IDLE_CONN = 4;
    const FLD_EA_WAITQUE_DEPTH = 5;
    const FLD_EA_REQ_PER_SEC = 6;
    const FLD_EA_TOT_REQS = 7;
    const FLD_EA_TYPE = 8;
    const COUNT_FLD_EA = 9;

    private $_serv;
    private $_vhosts;
    private $_rawdata;
    private $_blockedIpReport;

    protected function __construct()
    {
        $this->_rawdata = $this->loadRawData();
        if (!is_string($this->_rawdata)) {
            $this->_rawdata = '';
        }
        $this->_blockedIpReport = null;
    }

    protected function loadRawData()
    {
        $rawdata = '';
        $statsDir = $this->getStatsDir();
        if (!is_string($statsDir) || $statsDir === '') {
            return '';
        }

        $processes = (int) $this->getReportProcessCount();
        if ($processes < 1) {
            $processes = 1;
        }

        $rtrpt = rtrim($statsDir, '/') . '/.rtreport';
        for ($i = 1; $i <= $processes; $i ++) {
            if ($i > 1) {
                $rawdata .= "\n" . @file_get_contents("{$rtrpt}.{$i}");
            } else {
                $rawdata = @file_get_contents($rtrpt);
                if ($rawdata == false) {
                    break; // fail to open, may due to restart, ignore
                }
            }
        }

        return $rawdata;
    }

    protected function getStatsDir()
    {
        return '';
    }

    protected function getReportProcessCount()
    {
        return 1;
    }

    public static function GetDashPlot()
    {
        $stat = new static();
        $stat->parse_server();
        return $stat->_serv;
    }

    public static function GetPlotStats()
    {
        $stat = new static();
        $stat->parse_server();
        $stat->parse_plotvh();
        $stat->_rawdata = '';

        return $stat;
    }

    public static function GetVHStats()
    {
        $stat = new static();
        $stat->parse_vhosts();
        $stat->_rawdata = '';

        return $stat;
    }

    public static function GetBlockedIpReport()
    {
        $stat = new static();
        $report = $stat->parse_blocked_ips();
        $stat->_rawdata = '';

        return $report;
    }

    public static function GetBlockedIpReasonMap()
    {
        $class = static::GetBlockedIpReasonMapClass();

        return call_user_func(array($class, 'getMap'));
    }

    protected static function GetBlockedIpReasonMapClass()
    {
        return 'LSWebAdmin\\Product\\Base\\BlockedIpReasonMap';
    }

    public static function ParseBlockedIpReportFromRawData($rawdata)
    {
        $reasonMap = self::GetBlockedIpReasonMap();
        $items = [];
        $categories = [];
        $seen = [];
        $match = [];

        if (!is_string($rawdata) || $rawdata === '') {
            return [
                'count' => 0,
                'categories' => [],
                'items' => [],
            ];
        }

        if (preg_match_all("/^BLOCKED_IP: ([0-9 \\[\\]\\.,;:A-Za-f]*)/m", $rawdata, $match)) {
            foreach ($match[1] as $line) {
                $entries = preg_split("/[\\s,]+/", trim($line), -1, PREG_SPLIT_NO_EMPTY);
                if (!is_array($entries)) {
                    continue;
                }

                foreach ($entries as $entry) {
                    $entry = trim($entry, ", \t\n\r\0\x0B");
                    if ($entry === '') {
                        continue;
                    }

                    $parts = [];
                    if (preg_match('/^(.*)[:;]([A-Za-z?][A-Za-z0-9_-]*)$/', $entry, $parts)) {
                        $ip = trim($parts[1], "[] \t\n\r\0\x0B");
                        $code = strtoupper(trim($parts[2]));
                    } else {
                        $ip = trim($entry, "[] \t\n\r\0\x0B");
                        $code = '?';
                    }

                    if ($ip === '') {
                        continue;
                    }

                    if (!isset($reasonMap[$code])) {
                        $code = '?';
                    }

                    $key = strtolower($ip) . '|' . $code;
                    if (isset($seen[$key])) {
                        continue;
                    }
                    $seen[$key] = true;

                    if (!isset($categories[$code])) {
                        $categories[$code] = [
                            'code' => $code,
                            'reason' => isset($reasonMap[$code]) ? $reasonMap[$code] : 'UNKNOWN',
                            'count' => 0,
                        ];
                    }

                    $categories[$code]['count']++;
                    $items[] = [
                        'ip' => $ip,
                        'code' => $code,
                        'reason' => $categories[$code]['reason'],
                    ];
                }
            }
        }

        uasort($categories, function ($left, $right) {
            if ($left['count'] === $right['count']) {
                return strcmp($left['code'], $right['code']);
            }

            return ($left['count'] < $right['count']) ? 1 : -1;
        });

        return [
            'count' => count($items),
            'categories' => array_values($categories),
            'items' => $items,
        ];
    }

    public function GetServerData()
    {
        return $this->_serv;
    }

    public function GetVHData()
    {
        return $this->_vhosts;
    }

    private function parse_blocked_ips()
    {
        if ($this->_blockedIpReport === null) {
            $this->_blockedIpReport = self::ParseBlockedIpReportFromRawData($this->_rawdata);
        }

        return $this->_blockedIpReport;
    }

    private function parse_server()
    {
        $this->_serv = array_fill(0, self::COUNT_FLD_SERVER, 0);
        //error_log("rawdata = " . $this->_rawdata);

        $m = [];
        if ($found = preg_match_all("/^UPTIME: ([0-9A-Za-z\ \:]+)\nBPS_IN:([0-9\ ]+), BPS_OUT:([0-9\ ]+), SSL_BPS_IN:([0-9\ ]+), SSL_BPS_OUT:([0-9\ ]+)\nMAXCONN:([0-9\ ]+), MAXSSL_CONN:([0-9\ ]+), PLAINCONN:([0-9\ ]+), AVAILCONN:([0-9\ ]+), IDLECONN:([0-9\ ]+), SSLCONN:([0-9\ ]+), AVAILSSL:([0-9\ ]+)/m", $this->_rawdata, $m)) {
            for ($f = 0; $f < $found; $f ++) {
                $this->_serv[self::FLD_UPTIME] = trim($m[1][$f]);
                $this->_serv[self::FLD_BPS_IN] += (int) $m[2][$f];
                $this->_serv[self::FLD_BPS_OUT] += (int) $m[3][$f];
                $this->_serv[self::FLD_SSL_BPS_IN] += (int) $m[4][$f];
                $this->_serv[self::FLD_SSL_BPS_OUT] += (int) $m[5][$f];

                $this->_serv[self::FLD_MAXCONN] += (int) $m[6][$f];
                $this->_serv[self::FLD_MAXSSL_CONN] += (int) $m[7][$f];

                $this->_serv[self::FLD_PLAINCONN] += (int) $m[8][$f];
                $this->_serv[self::FLD_AVAILCONN] += (int) $m[9][$f];
                $this->_serv[self::FLD_IDLECONN] += (int) $m[10][$f];

                $this->_serv[self::FLD_SSLCONN] += (int) $m[11][$f];
                $this->_serv[self::FLD_AVAILSSL] += (int) $m[12][$f];
            }
        }

        $blockedIpReport = $this->parse_blocked_ips();
        $this->_serv[self::FLD_BLOCKEDIP_COUNT] = $blockedIpReport['count'];


        $m = [];
        if ($found = preg_match_all("/^REQ_RATE \[\]: REQ_PROCESSING: ([0-9]+), REQ_PER_SEC: ([0-9\.]+), TOT_REQS: ([0-9]+)(?:, PUB_CACHE_HITS_PER_SEC: ([0-9\.]+), TOTAL_PUB_CACHE_HITS: ([0-9]+), PRIVATE_CACHE_HITS_PER_SEC: ([0-9\.]+), TOTAL_PRIVATE_CACHE_HITS: ([0-9]+), STATIC_HITS_PER_SEC: ([0-9\.]+), TOTAL_STATIC_HITS: ([0-9]+))?/m", $this->_rawdata, $m)) {
            for ($f = 0; $f < $found; $f ++) {
                $this->_serv[self::FLD_S_REQ_PROCESSING] += (int) $m[1][$f];
                $this->_serv[self::FLD_S_REQ_PER_SEC] += doubleval($m[2][$f]);
                $this->_serv[self::FLD_S_TOT_REQS] += doubleval($m[3][$f]);
                $this->_serv[self::FLD_S_PUB_CACHE_HITS_PER_SEC] += self::toFloat($m[4][$f]);
                $this->_serv[self::FLD_S_TOTAL_PUB_CACHE_HITS] += self::toFloat($m[5][$f]);
                $this->_serv[self::FLD_S_PRIV_CACHE_HITS_PER_SEC] += self::toFloat($m[6][$f]);
                $this->_serv[self::FLD_S_TOTAL_PRIV_CACHE_HITS] += self::toFloat($m[7][$f]);
                $this->_serv[self::FLD_S_STATIC_HITS_PER_SEC] += self::toFloat($m[8][$f]);
                $this->_serv[self::FLD_S_TOTAL_STATIC_HITS] += self::toFloat($m[9][$f]);
            }
        }
    }

    private function parse_plotvh()
    {
        $this->_vhosts = [];

        $vhmonitored = null;
        if (isset($_REQUEST['vhnames']) && is_array($_REQUEST['vhnames']))
            $vhmonitored = $_REQUEST['vhnames'];
        else
            return;

        $vhosts = [];
        $m = [];
        $found = preg_match_all("/REQ_RATE \[(.+)\]: REQ_PROCESSING: ([0-9]+), REQ_PER_SEC: ([0-9\.]+), TOT_REQS: ([0-9]+)(?:, PUB_CACHE_HITS_PER_SEC: ([0-9\.]+), TOTAL_PUB_CACHE_HITS: ([0-9]+), PRIVATE_CACHE_HITS_PER_SEC: ([0-9\.]+), TOTAL_PRIVATE_CACHE_HITS: ([0-9]+), STATIC_HITS_PER_SEC: ([0-9\.]+), TOTAL_STATIC_HITS: ([0-9]+))?/i", $this->_rawdata, $m);

        for ($f = 0; $f < $found; $f ++) {
            $vhname = trim($m[1][$f]);

            if ($vhmonitored != null && !in_array($vhname, $vhmonitored))
                continue;

            if (!isset($vhosts[$vhname])) {
                $vhosts[$vhname] = array_fill(0, self::COUNT_FLD_VH, 0);
                $vhosts[$vhname]['ea'] = [];
            }
            $vh = &$vhosts[$vhname];
            $vh[self::FLD_VH_REQ_PROCESSING] += (int) $m[2][$f];
            $vh[self::FLD_VH_REQ_PER_SEC] += doubleval($m[3][$f]);
            $vh[self::FLD_VH_TOT_REQS] += doubleval($m[4][$f]);
            $vh[self::FLD_VH_PUB_CACHE_HITS_PER_SEC] += self::toFloat($m[5][$f]);
            $vh[self::FLD_VH_TOTAL_PUB_CACHE_HITS] += self::toFloat($m[6][$f]);
            $vh[self::FLD_VH_PRIV_CACHE_HITS_PER_SEC] += self::toFloat($m[7][$f]);
            $vh[self::FLD_VH_TOTAL_PRIV_CACHE_HITS] += self::toFloat($m[8][$f]);
            $vh[self::FLD_VH_STATIC_HITS_PER_SEC] += self::toFloat($m[9][$f]);
            $vh[self::FLD_VH_TOTAL_STATIC_HITS] += self::toFloat($m[10][$f]);
        }

        //reset
        $m = [];
        $found = preg_match_all("/EXTAPP \[(.+)\] \[(.+)\] \[(.+)\]: CMAXCONN: ([0-9]+), EMAXCONN: ([0-9]+), POOL_SIZE: ([0-9]+), INUSE_CONN: ([0-9]+), IDLE_CONN: ([0-9]+), WAITQUE_DEPTH: ([0-9]+), REQ_PER_SEC: ([0-9\.]+), TOT_REQS: ([0-9]+)/i", $this->_rawdata, $m);

        for ($f = 0; $f < $found; $f ++) {
            $vhname = trim($m[2][$f]);
            $extapp = trim($m[3][$f]);

            if ($vhname == '') {
                $vhname = '_Server';
                if (!isset($vhosts[$vhname])) {
                    $vhosts[$vhname] = array_fill(0, self::COUNT_FLD_VH, 0);
                    $vhosts[$vhname]['ea'] = [];
                }
            } else {
                if (!isset($vhosts[$vhname]))
                    continue;
            }

            if (!isset($vhosts[$vhname]['ea'][$extapp])) {
                $vhosts[$vhname][self::FLD_VH_EAP_COUNT] ++;
                $vhosts[$vhname]['ea'][$extapp] = 1;
            }

            $vhosts[$vhname][self::FLD_VH_EAP_INUSE] += (int) $m[7][$f];
            $vhosts[$vhname][self::FLD_VH_EAP_IDLE] += (int) $m[8][$f];
            $vhosts[$vhname][self::FLD_VH_EAP_WAITQUE] += (int) $m[9][$f];
            $vhosts[$vhname][self::FLD_VH_EAP_REQ_PER_SEC] += doubleval($m[10][$f]);
        }

        $this->_vhosts = $vhosts;
    }

    private function parse_vhosts()
    {
        $this->_vhosts = [];

        $top = UIBase::GrabGoodInput("request", "vh_topn", 'int');
        $filter = UIBase::GrabGoodInput("request", "vh_filter", "string");
        $sort = UIBase::GrabGoodInput("request", "vh_sort", "int");

        $vhosts = [];
        $m = [];
        $found = preg_match_all("/REQ_RATE \[(.+)\]: REQ_PROCESSING: ([0-9]+), REQ_PER_SEC: ([0-9\.]+), TOT_REQS: ([0-9]+)(?:, PUB_CACHE_HITS_PER_SEC: ([0-9\.]+), TOTAL_PUB_CACHE_HITS: ([0-9]+), PRIVATE_CACHE_HITS_PER_SEC: ([0-9\.]+), TOTAL_PRIVATE_CACHE_HITS: ([0-9]+), STATIC_HITS_PER_SEC: ([0-9\.]+), TOTAL_STATIC_HITS: ([0-9]+))?/i", $this->_rawdata, $m);

        for ($f = 0; $f < $found; $f ++) {
            $vhname = trim($m[1][$f]);

            if ($filter != "" && (!preg_match("/$filter/i", $vhname)))
                continue;

            if (!isset($vhosts[$vhname])) {
                $vhosts[$vhname] = array_fill(0, self::COUNT_FLD_VH, 0);
                $vhosts[$vhname]['ea'] = [];
            }
            $vh = &$vhosts[$vhname];
            $vh[self::FLD_VH_REQ_PROCESSING] += (int) $m[2][$f];
            $vh[self::FLD_VH_REQ_PER_SEC] += doubleval($m[3][$f]);
            $vh[self::FLD_VH_TOT_REQS] += doubleval($m[4][$f]);
            $vh[self::FLD_VH_PUB_CACHE_HITS_PER_SEC] += self::toFloat($m[5][$f]);
            $vh[self::FLD_VH_TOTAL_PUB_CACHE_HITS] += self::toFloat($m[6][$f]);
            $vh[self::FLD_VH_PRIV_CACHE_HITS_PER_SEC] += self::toFloat($m[7][$f]);
            $vh[self::FLD_VH_TOTAL_PRIV_CACHE_HITS] += self::toFloat($m[8][$f]);
            $vh[self::FLD_VH_STATIC_HITS_PER_SEC] += self::toFloat($m[9][$f]);
            $vh[self::FLD_VH_TOTAL_STATIC_HITS] += self::toFloat($m[10][$f]);
        }

        //reset
        $m = [];
        $found = preg_match_all("/EXTAPP \[(.+)\] \[(.+)\] \[(.+)\]: CMAXCONN: ([0-9]+), EMAXCONN: ([0-9]+), POOL_SIZE: ([0-9]+), INUSE_CONN: ([0-9]+), IDLE_CONN: ([0-9]+), WAITQUE_DEPTH: ([0-9]+), REQ_PER_SEC: ([0-9\.]+), TOT_REQS: ([0-9]+)/i", $this->_rawdata, $m);

        for ($f = 0; $f < $found; $f ++) {
            $vhname = trim($m[2][$f]);
            $extapp = trim($m[3][$f]);

            if ($vhname == '') {
                $vhname = '_Server';
                if (!isset($vhosts[$vhname])) {
                    $vhosts[$vhname] = array_fill(0, self::COUNT_FLD_VH, 0);
                    $vhosts[$vhname]['ea'] = [];
                }
            } else {
                if (!isset($vhosts[$vhname]))
                    continue;
            }

            if (!isset($vhosts[$vhname]['ea'][$extapp])) {
                $vhosts[$vhname][self::FLD_VH_EAP_COUNT] ++;
                $vhosts[$vhname]['ea'][$extapp] = array_fill(0, self::COUNT_FLD_EA, 0);
            }

            $ea = &$vhosts[$vhname]['ea'][$extapp];
            $ea[self::FLD_EA_TYPE] = trim($m[1][$f]);
            $ea[self::FLD_EA_CMAXCONN] += (int) $m[4][$f];
            $ea[self::FLD_EA_EMAXCONN] += (int) $m[5][$f];
            $ea[self::FLD_EA_POOL_SIZE] += (int) $m[6][$f];
            $ea[self::FLD_EA_INUSE_CONN] += (int) $m[7][$f];
            $ea[self::FLD_EA_IDLE_CONN] += (int) $m[8][$f];
            $ea[self::FLD_EA_WAITQUE_DEPTH] += (int) $m[9][$f];
            $ea[self::FLD_EA_REQ_PER_SEC] += doubleval($m[10][$f]);
            $ea[self::FLD_EA_TOT_REQS] += doubleval($m[11][$f]);

            $vhosts[$vhname][self::FLD_VH_EAP_INUSE] += (int) $m[7][$f];
            $vhosts[$vhname][self::FLD_VH_EAP_IDLE] += (int) $m[8][$f];
            $vhosts[$vhname][self::FLD_VH_EAP_WAITQUE] += (int) $m[9][$f];
            $vhosts[$vhname][self::FLD_VH_EAP_REQ_PER_SEC] += doubleval($m[10][$f]);
        }

        $sortDesc1 = [];
        $sortAsc2 = [];
        $names = array_keys($vhosts);
        if ($sort != "" && count($names) > 1) {
            foreach ($names as $vhname) {
                if ($vhosts[$vhname][$sort] > 0) {
                    $sortDesc1[] = $vhosts[$vhname][$sort];
                    $sortAsc2[] = $vhname;
                }
            }
            if (count($sortAsc2) > 0) {
                array_multisort($sortDesc1, SORT_DESC, SORT_NUMERIC, $sortAsc2, SORT_ASC, SORT_STRING);
                $names = $sortAsc2;
            }
        }

        if ($top != 0 && count($names) > $top) {
            $names = array_slice($names, 0, $top);
        }

        foreach ($names as $vn) {
            $this->_vhosts[$vn] = $vhosts[$vn];
        }
    }

    private static function toFloat($value)
    {
        if ($value === '' || $value === null) {
            return 0;
        }

        return doubleval($value);
    }

}
