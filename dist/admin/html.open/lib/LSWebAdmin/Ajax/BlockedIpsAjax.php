<?php

namespace LSWebAdmin\Ajax;

use LSWebAdmin\Product\Current\RealTimeStats;
use LSWebAdmin\UI\UIBase;

class BlockedIpsAjax
{
    public static function preview()
    {
        $report = self::getReport();
        $previewItems = array_slice($report['items'], 0, 10);

        echo json_encode([
            'total_count' => $report['count'],
            'preview_count' => count($previewItems),
            'has_more' => ($report['count'] > count($previewItems)),
            'categories' => $report['categories'],
            'items' => $previewItems,
            'full_url' => self::fullViewUrl(),
        ]);
    }

    public static function full()
    {
        $report = self::getReport();

        echo json_encode([
            'total_count' => $report['count'],
            'categories' => $report['categories'],
            'items' => $report['items'],
            'full_url' => self::fullViewUrl(),
        ]);
    }

    public static function download()
    {
        $format = strtolower((string) UIBase::GrabGoodInput('get', 'format'));
        $search = (string) UIBase::GrabGoodInput('get', 'search');
        $reason = (string) UIBase::GrabGoodInput('get', 'reason');
        $report = self::getReport();
        $items = self::filterItems($report['items'], $search, $reason);
        $timestamp = date('Ymd_His');

        if ($format !== 'txt') {
            $format = 'csv';
        }

        if (ob_get_level()) {
            ob_end_clean();
        }

        if ($format === 'csv') {
            header('Content-Type: text/csv; charset=UTF-8');
            header('Content-Disposition: attachment; filename=blocked_ips_' . $timestamp . '.csv');

            $output = fopen('php://output', 'w');
            if ($output === false) {
                return;
            }

            fputcsv($output, ['IP Address', 'Reason Code', 'Reason']);
            foreach ($items as $item) {
                fputcsv($output, [
                    isset($item['ip']) ? $item['ip'] : '',
                    isset($item['code']) ? $item['code'] : '',
                    isset($item['reason']) ? $item['reason'] : '',
                ]);
            }
            fclose($output);
            exit;
        }

        header('Content-Type: text/plain; charset=UTF-8');
        header('Content-Disposition: attachment; filename=blocked_ips_' . $timestamp . '.txt');

        foreach ($items as $item) {
            echo (isset($item['ip']) ? $item['ip'] : '')
                . "\t" . (isset($item['code']) ? $item['code'] : '')
                . "\t" . (isset($item['reason']) ? $item['reason'] : '')
                . "\n";
        }
        exit;
    }

    private static function getReport()
    {
        return RealTimeStats::GetBlockedIpReport();
    }

    private static function filterItems($items, $search = '', $reason = '')
    {
        $search = trim((string) $search);
        $reason = strtoupper(trim((string) $reason));

        if ($search === '' && $reason === '') {
            return $items;
        }

        return array_values(array_filter($items, function ($item) use ($search, $reason) {
            if ($reason !== '' && (!isset($item['code']) || strtoupper($item['code']) !== $reason)) {
                return false;
            }

            if ($search === '') {
                return true;
            }

            $haystack = implode(' ', [
                isset($item['ip']) ? $item['ip'] : '',
                isset($item['code']) ? $item['code'] : '',
                isset($item['reason']) ? $item['reason'] : '',
            ]);

            return stripos($haystack, $search) !== false;
        }));
    }

    private static function fullViewUrl()
    {
        return 'index.php?view=blockedips';
    }
}
