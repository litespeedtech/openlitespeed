<?php 

global $_tipsdb;

$_tipsdb['AIOBlockSize'] = new DAttrHelp("AIOブロックサイズ", 'AIOの送信ブロックサイズを指定します。 このブロックサイズに処理中の合計ファイル数を掛けた値は、サーバーの物理メモリより小さくする必要があります。そうでない場合、AIOは役に立ちません。サーバーに十分なメモリがある場合は、より大きなサイズを選択できます。<br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>128K</span>', '', 'ドロップダウンリストから選択', '');

$_tipsdb['CACertFile'] = new DAttrHelp("CA証明書ファイル", 'チェーン証明書の証明機関（CA）のすべての証明書を含むファイルを指定します。 このファイルは、PEMでエンコードされた証明書ファイルを単に優先順に連結したものです。 これは、「CA証明書パス」の代替として、またはこれに加えて使用することができる。 これらの証明書は、クライアント証明書の認証およびサーバー証明書チェーンの構築に使用されます。サーバー証明書チェーンは、サーバー証明書に加えてブラウザーにも送信されます。', '', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['CACertPath'] = new DAttrHelp("CA証明書パス", '証明機関（CA）の証明書が保存されるディレクトリを指定します。 これらの証明書は、クライアント証明書の認証およびサーバー証明書チェーンの構築に使用されます。サーバー証明書チェーンは、サーバー証明書に加えてブラウザーにも送信されます。', '', 'パス', '');

$_tipsdb['CGIPriority'] = new DAttrHelp("CGI優先度", '外部アプリケーションプロセスの優先度を指定します。値の範囲は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-20</span>から<span class=&quot;lst-inline-token lst-inline-token--value&quot;>20</span>です。数値が小さいほど優先度は高くなります。<br/><br/>CGIプロセスはWebサーバーより高い優先度を持つことはできません。この優先度がサーバーの優先度より小さい数値に設定されている場合、サーバーの優先度がこの値に使用されます。', '', '整数', '');

$_tipsdb['CPUHardLimit'] = new DAttrHelp("CPUハードリミット(秒)", 'CGIプロセスの最大CPU使用時間制限を秒単位で指定します。プロセスがCPU時間を消費してハードリミットに達すると、プロセスは強制終了されます。 値が存在しないか、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定されている場合は、オペレーティングシステムのデフォルト設定が使用されます。', '', '整数', '');

$_tipsdb['CPUSoftLimit'] = new DAttrHelp("CPUソフトリミット(秒)", 'CGIプロセスのCPU消費制限時間を秒単位で指定します。 プロセスがソフトリミットに達すると、シグナルによって通知されます。 値が存在しない場合、または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定されている場合は、オペレーティングシステムのデフォルト設定が使用されます。.', '', '整数', '');

$_tipsdb['DHParam'] = new DAttrHelp("DHパラメータ", 'DHキー交換に必要なDiffie-Hellmanパラメータファイルの場所を指定します。', '', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['GroupDBLocation'] = new DAttrHelp("グループDBの場所", 'グループデータベースの場所を指定します。データベースは$SERVER_ROOT/conf/vhosts/$VH_NAME/ディレクトリの下に保存することをお勧めします。<br/><br/>グループ情報は、ユーザーデータベースまたはこの独立したグループDBのどちらにも設定できます。ユーザー認証では、最初にユーザーDBがチェックされます。ユーザーDBにもグループ情報が含まれている場合、グループDBはチェックされません。<br/><br/>DBタイプが<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Password File</span>の場合、グループDBの場所はグループ定義を含むフラットファイルへのパスにする必要があります。ファイル名をクリックすると、WebAdminコンソールからこのファイルを編集できます。<br/><br/>グループファイルの各行には、グループ名、コロン、スペース区切りのユーザー名グループが含まれている必要があります。例:<br/><blockquote><code>testgroup: user1 user2 user3</code></blockquote>', '', 'ファイル名への絶対パス又は$SERVER_ROOT、$VH_ROOTからの相対パス。', '');

$_tipsdb['HANDLER_RESTART'] = new DAttrHelp("フック::HANDLER_RESTART 優先度", 'このモジュールコールバックの優先度をHTTPハンドラの再起動フック内で設定します。<br/><br/>Webサーバが現在の応答を破棄し、内部リダイレクトが要求されたときなど、最初から処理を開始する必要があるときに、HTTPハンドラの再起動フックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['HTTP_AUTH'] = new DAttrHelp("フック::HTTP_AUTH 優先度", 'HTTP認証フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>HTTP認証フックは、リソースマッピング後およびハンドラ処理の前にトリガーされます。 HTTP組み込み認証の後に発生し、追加の認証チェックを実行するために使用できます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['HTTP_BEGIN'] = new DAttrHelp("フック::HTTP_BEGIN 優先度", 'HTTP Begin フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>TCP/IP接続がHTTPセッションを開始すると、HTTP Begin フックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['HTTP_END'] = new DAttrHelp("フック::HTTP_END 優先度", 'HTTPセッション終了フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>HTTPセッション終了フックは、HTTP接続が終了したときにトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['L4_BEGINSESSION'] = new DAttrHelp("フック::L4_BEGINSESSION 優先度", 'このモジュールコールバックの優先度をL4 Begin Session フック内で設定します。<br/><br/>TCP/IP接続が開始されると、L4 Begin Session フックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['L4_ENDSESSION'] = new DAttrHelp("フック::L4_ENDSESSION 優先度", 'このモジュールコールバックの優先度をL4 End Session フック内で設定します。<br/><br/>TCP/IP接続が終了すると、L4 End Session フックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['L4_RECVING'] = new DAttrHelp("フック::L4_RECVING 優先度", 'L4 Receiving フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>TCP/IP接続がデータを受信すると、L4 Receiving フックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['L4_SENDING'] = new DAttrHelp("フック::L4_SENDING 優先度", 'このモジュールコールバックの優先度をL4 Sendingフック内で設定します。<br/><br/>L4_SENDINGフックは、TCP/IP接続がデータを送信するときにトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['MAIN_ATEXIT'] = new DAttrHelp("フック::MAIN_ATEXIT 優先度", 'メインの出口フック内のこのモジュールコールバックの優先度を設定します<br/><br/>メインの出口フックは、終了する直前のメイン（コントローラ）プロセスによって起動されます。 これは、メインプロセスによって呼び出される最後のフックポイントです。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['MAIN_INITED'] = new DAttrHelp("フック::MAIN_INITED 優先度", 'メイン初期化フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>メイン初期化フックは、起動時、メイン（コントローラ）プロセスによってサーバー構成と初期化が完了した後、および要求が処理される前に一度起動されます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先順位はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['MAIN_POSTFORK'] = new DAttrHelp("フック::MAIN_POSTFORK 優先度", 'Main Postfork フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>Main Postfork フックは、新しいワーカープロセスが開始（フォーク）された直後にメイン（コントローラ）プロセスによってトリガーされます。 これは各ワーカーに呼び出され、システムの起動時やワーカが再起動されたときに発生する可能性があります。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['MAIN_PREFORK'] = new DAttrHelp("フック::MAIN_PREFORK 優先度", 'Main Prefork フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>Main Prefork フックは、新しいワーカープロセスが開始（分岐）される直前にメイン（コントローラ）プロセスによってトリガーされます。 これは各ワーカーに呼び出され、システムの起動時やワーカが再起動されたときに発生する可能性があります。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['RCVD_REQ_BODY'] = new DAttrHelp("フック::RCVD_REQ_BODY 優先度", 'HTTP受信済みリクエストボディフック内のこのモジュールコールバックの優先度を設定します。<br/><br/>Webサーバーが要求本体データの受信を終了すると、HTTP受信要求本体フックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['RCVD_RESP_BODY'] = new DAttrHelp("フック::RCVD_RESP_BODY 優先度", 'HTTP受信応答ボディフック内のこのモジュールコールバックの優先度を設定します。<br/><br/>HTTP受信応答ボディフックは、Webサーバーのバックエンドが応答本体の受信を終了するとトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['RECV_REQ_BODY'] = new DAttrHelp("フック::RECV_REQ_BODY 優先度", 'HTTP受信要求ボディフック内のこのモジュールコールバックの優先度を設定します。<br/><br/>Webサーバーが要求本体データを受信すると、HTTP受信要求ボディフックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['RECV_REQ_HEADER'] = new DAttrHelp("フック::RECV_REQ_HEADER 優先度", 'HTTP受信要求ヘッダーフック内のこのモジュールコールバックの優先度を設定します。<br/><br/>HTTP受信要求ヘッダーフックは、Webサーバーが要求ヘッダーを受信するとトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['RECV_RESP_BODY'] = new DAttrHelp("フック::RECV_RESP_BODY 優先度", 'HTTP受信応答ボディフック内のこのモジュールコールバックの優先度を設定します。<br/><br/>HTTP受信応答ボディフックは、Webサーバーのバックエンドが応答本体を受信するとトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['RECV_RESP_HEADER'] = new DAttrHelp("フック::RECV_RESP_HEADER 優先度", 'HTTP受信応答ヘッダーフック内のこのモジュールコールバックの優先度を設定します。<br/><br/>HTTP受信応答ヘッダーフックは、Webサーバーが応答ヘッダーを作成するときにトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['SEND_RESP_BODY'] = new DAttrHelp("フック::SEND_RESP_BODY 優先度", 'HTTP送信レスポンスボディフック内のこのモジュールコールバックの優先度を設定します。<br/><br/>HTTP送信レスポンスボディフックは、Webサーバがレスポンスボディを送信するときにトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['SEND_RESP_HEADER'] = new DAttrHelp("フック::SEND_RESP_HEADER 優先度", 'このモジュールコールバックの優先度を、HTTP送信応答ヘッダーフック内で設定します。<br/><br/>Webサーバーが応答ヘッダーを送信する準備ができたら、HTTP送信応答ヘッダーフックがトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['UDBgroup'] = new DAttrHelp("グループ", 'このユーザーが属するグループのカンマ区切りリストです。ユーザーはこれらのグループに属するリソースにのみアクセスできます。<br/><br/>ここにグループ情報を追加すると、この情報がリソース認可に使用され、このユーザーに関係するグループDB設定は無視されます。', '', '', '');

$_tipsdb['UDBpass'] = new DAttrHelp("新しいパスワード", 'パスワードは任意の長さで、任意の文字を含めることができます。', '', '', '');

$_tipsdb['UDBpass1'] = new DAttrHelp("パスワード再入力", 'パスワードは任意の長さで、任意の文字を含めることができます。', '', '', '');

$_tipsdb['UDBusername'] = new DAttrHelp("ユーザー名", '英字と数字のみを含むユーザー名です（特殊文字は使用できません）。', '', '', '');

$_tipsdb['URI_MAP'] = new DAttrHelp("フック::URI_MAP 優先度", 'このモジュールコールバックの優先度をHTTP URIマップフック内で設定します。<br/><br/>HTTP URI マップフックは、WebサーバーがURI要求をコンテキストにマップするときにトリガーされます。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['VHlsrecaptcha'] = new DAttrHelp("CAPTCHA保護", 'CAPTCHA保護は、高いサーバー負荷を軽減する方法として提供されるサービスです。以下のいずれかの状況に達すると、CAPTCHA保護が有効になります。有効になると、非信頼クライアント（設定による）からのすべてのリクエストはCAPTCHA検証ページにリダイレクトされます。検証後、クライアントは目的のページにリダイレクトされます。<br/><br/>CAPTCHA保護は次の状況で有効になります:<br/>1. サーバーまたはバーチャルホストの同時リクエスト数が、設定された接続制限を超えた場合。<br/>2. Anti-DDoSが有効で、クライアントが疑わしい方法でURLにアクセスしている場合。トリガーされると、クライアントは拒否される代わりに先にCAPTCHAへリダイレクトされます。<br/>3. CAPTCHAをRewriteRules経由で有効化するための新しいRewriteルール環境が提供されています。&#039;verifycaptcha&#039;を設定すると、クライアントをCAPTCHAへリダイレクトできます。特別な値&#039;: deny&#039;を設定すると、失敗回数が多すぎるクライアントを拒否できます。たとえば、[E=verifycaptcha]は検証されるまで常にCAPTCHAへリダイレクトします。[E=verifycaptcha: deny]はMax Triesに達するまでCAPTCHAへリダイレクトし、その後クライアントを拒否します。', '', '', '');

$_tipsdb['WORKER_ATEXIT'] = new DAttrHelp("フック::WORKER_ATEXIT 優先度", 'このモジュールコールバックの優先度を、終了時のワーカーのフック内で設定します。<br/><br/>退出時のワーカーは、退出する直前のワーカープロセスによってトリガーされます。 これは、ワーカーによって呼び出される最後のフックポイントです。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['WORKER_POSTFORK'] = new DAttrHelp("フック::WORKER_POSTFORK 優先度", 'Worker Postfork フック内のこのモジュールコールバックの優先度を設定します。<br/><br/>Worker Postfork フックは、メイン（コントローラ）プロセスによって作成された後、ワーカープロセスによってトリガーされます。 対応するMain Postfork フックは、このコールバックの前または後のメインプロセスによって呼び出されることに注意してください。<br/><br/>モジュールにフックポイントがある場合にのみ有効です。 設定されていない場合、優先度はモジュールで定義されたデフォルト値になります。', '', '整数値は-6000から6000です。値が小さいほど優先度が高くなります。', '');

$_tipsdb['accessAllowed'] = new DAttrHelp("アクセス許可", 'このコンテキストでリソースにアクセスできるIPまたはサブネットワークを指定します。 &quot;アクセス拒否&quot;とサーバー/バーチャルホスト・レベルのアクセス制御とともに、アクセシビリティは、クライアントのIPアドレスが入る最小の範囲によって決まります。', '', 'IP/サブネットワークのカンマ区切りリスト。', 'サブネットワークは以下のように書くことができます <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/255.255.255.0</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1</span>, 又は <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*</span>.');

$_tipsdb['accessControl'] = new DAttrHelp("アクセス制御", 'どのサブネットワークおよび/またはIPアドレスがサーバーにアクセスできるかを指定します。 サーバレベルでは、この設定はすべてのバーチャルホストに影響します。 バーチャルホストレベルで各バーチャルホストに固有のアクセス制御を設定することもできます。 バーチャルホストレベルの設定はサーバーレベルの設定を上書きしません。<br/><br/>ブロック/ IPの許可は、許可リストと拒否リストの組み合わせによって決まります。 特定のIPまたはサブネットワークのみをブロックする場合は、&quot;許可リスト&quot;に<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>を入れ、ブロックされたIPまたはサブネットワークを&quot;拒否リスト&quot;。<br/>特定のIPまたはサブネットワークのみを許可する場合は、&quot;拒否リスト&quot;に<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>を入れ、許可されたIPまたはサブネットワークを&quot;許可リスト&quot;。<br/>IPに適合する最小スコープの設定は、アクセスを決定するために使用されます。<br/><br/><b>サーバーレベル：</b>信頼できるIPまたはサブネットワークは、&quot;許可リスト&quot;に、末尾の &quot;T&quot;を追加することで指定する必要があります。 信頼できるIPまたはサブネットワークは、接続/スロットリング制限の影響を受けません。 信頼できるIP/サブネットワークは、サーバーレベルのアクセス制御でのみ設定できます。', '[セキュリティ]すべてのバーチャルホストに適用される一般的な制限については、サーバーレベルでこれを使用してください。', '', '');

$_tipsdb['accessControl_allow'] = new DAttrHelp("許可リスト", '許可されるIPまたはサブネットワークのリストを指定します。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>が受け入れられます。', '[セキュリティ]サーバーレベルのアクセス制御で設定された信頼されたIPまたはサブネットワークは、接続/スロットリングの制限から除外されます。', 'IPアドレスまたはサブネットワークのカンマ区切りリスト。 末尾の「T」は、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*T</span>などの信頼できるIPまたはサブネットワークを示すために使用できます。', '<b>Sub-networks:</b> 192.168.1.0/255.255.255.0, 192.168.1.0/24, 192.168.1, or 192.168.1.*<br/><b>IPv6 addresses:</b> ::1 or [::1]<br/><b>IPv6 subnets:</b> 3ffe:302:11:2:20f:1fff:fe29:717c/64 or [3ffe:302:11:2:20f:1fff:fe29:717c]/64');

$_tipsdb['accessControl_deny'] = new DAttrHelp("拒否リスト", '許可されていないIPまたはサブネットワークのリストを指定します。', '', 'IPアドレスまたはサブネットワークのカンマ区切りリスト。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>が受け入れられます。', '<b>Sub-networks:</b> 192.168.1.0/255.255.255.0, 192.168.1.0/24, 192.168.1, or 192.168.1.*<br/><b>IPv6 addresses:</b> ::1 or [::1]<br/><b>IPv6 subnets:</b> 3ffe:302:11:2:20f:1fff:fe29:717c/64 or [3ffe:302:11:2:20f:1fff:fe29:717c]/64');

$_tipsdb['accessDenied'] = new DAttrHelp("アクセス拒否", 'このコンテキストでリソースにアクセスできないIPまたはサブネットワークを指定します。 &quot;アクセス許可&quot;とサーバー/バーチャルホストレベルのアクセス制御とともに、アクセシビリティはクライアントのIPアドレスが入る最小の範囲で決まります。', '', 'IP/サブネットワークのカンマ区切りリスト。', 'サブネットワークは以下のように書くことができます <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/255.255.255.0</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1</span>, 又は <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*</span>.');

$_tipsdb['accessDenyDir'] = new DAttrHelp("アクセスが拒否されたディレクトリ", 'アクセスをブロックするディレクトリを指定します。 重要なデータを含むディレクトリをこのリストに追加して、誤って機密ファイルをクライアントに公開しないようにします。 すべてのサブディレクトリを含めるためにパスに「*」を追加します。 &quot;シンボリックリンクに従う&quot;と&quot;シンボリックリンクを確認する&quot;の両方を有効にすると、拒否されたディレクトリに対してシンボリックリンクがチェックされます。', '[セキュリティ]重要な点：この設定では、これらのディレクトリから静的ファイルを提供することができません。 これは、PHP/Ruby/CGIなどの外部スクリプトによるエクスポージャーを防ぐものではありません。', 'ディレクトリのカンマ区切りリスト', '');

$_tipsdb['accessLog_bytesLog'] = new DAttrHelp("バイトログ", '帯域幅バイトのログファイルへのパスを指定します。 指定すると、cPanel互換の帯域幅ログが作成されます。 これにより、要求と応答本体の両方を含む要求に対して転送された合計バイトが記録されます。', ' ログファイルを別のディスクに配置します。', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['accessLog_fileName'] = new DAttrHelp("ファイル名", 'アクセスログファイル名。', '[パフォーマンス] アクセスログファイルは別のディスクに配置してください。', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['accessLog_logFormat'] = new DAttrHelp("ログ形式", ' アクセスログのログ形式を指定します。 ログフォーマットが設定されると、&quot;ログヘッダー&quot;の設定より優先されます。', '', '文字列。ログ形式の構文はApache 2.0のカスタム <a href="http://httpd.apache.org/docs/current/mod/mod_log_config.html#formats" target="_blank" rel="noopener noreferrer">ログ形式</a>と互換性があります。', '<br> <b>共通ログフォーマット（CLF）</b><br/>    &quot;%h %l %u %t \&quot;%r\&quot; %>s %b&quot;<br/><br/><b>バーチャルホストによる共通ログフォーマット</b><br/>    &quot;%v %h %l %u %t \&quot;%r\&quot; %>s %b&quot;<br/><br/><b>NCSA拡張/結合ログフォーマット</b><br/>    &quot;%h %l %u %t \&quot;%r\&quot; %>s %b \&quot;%{Referer}i\&quot; \&quot;%{User-agent}i\&quot;<br/><br/><b>FoobarのログCookie値</b><br/>    &quot;%{Foobar}C&quot;');

$_tipsdb['accessLog_logHeader'] = new DAttrHelp("ログヘッダー", 'HTTPリクエストヘッダー<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Referer</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>UserAgent</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Host</span>を記録するかどうかを指定します。', '[パフォーマンス] これらのヘッダーがアクセスログに不要な場合は、この機能をオフにします。', 'チェックボックス', '');

$_tipsdb['accessLog_pipedLogger'] = new DAttrHelp("パイプロガー", 'LiteSpeedがSTDINストリーム上のパイプを通じて送信するアクセスログデータを受け取る外部アプリケーションを指定します（ファイルハンドルは0）。 このフィールドが指定されると、アクセスログはロガーアプリケーションにのみ送信され、前のエントリで指定されたアクセスログファイルには送信されません。<br/><br/>ロガーアプリケーションは、最初に&quot;外部アプリケーション&quot;セクションで定義する必要があります。 サーバーレベルのアクセスログは、サーバーレベルで定義された外部ロガーアプリケーションのみを使用できます。 バーチャルホストレベルのアクセスロギングでは、バーチャルホストレベルで定義されたロガーアプリケーションのみを使用できます。<br/><br/>ロガープロセスは、他の外部（CGI/FastCGI/LSAPI）プロセスと同じ方法で生成されます。 つまり、バーチャルホストの&quot;外部アプリSet UIDモード&quot;設定で指定されたユーザーIDとして実行され、特権ユーザーとして実行されることはありません。<br/><br/>LiteSpeed Webサーバーは、ロガーアプリケーションのインスタンスが複数設定されている場合、複数のロガーアプリケーション間で簡単なロードバランシングを実行します。 LiteSpeedサーバーは、常にロガーアプリケーションの数を可能な限り少なく保とうとします。 1つのロガーアプリケーションがアクセスログエントリを時間内に処理できない場合にのみ、サーバーはロガーアプリケーションの別のインスタンスを生成しようとします。<br/><br/>ロガーがクラッシュした場合、Webサーバーは別のインスタンスを開始しますが、ストリームバッファーのログデータは失われます。 外部ロガーがログストリームの速度と量に追いつけない場合、ログデータを失う可能性があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['aclogUseServer'] = new DAttrHelp("ログ制御", 'アクセスログを書き込む場所を指定します。3つのオプションがあります: <ol> <li>サーバーのアクセスログに書き込む</li> <li>このバーチャルホストのアクセスログを作成する</li> <li>アクセスログを無効にする</li> </ol>', '', '選択', '');

$_tipsdb['acme'] = new DAttrHelp("ACMEによるAutoCert", 'Automatic Certificate Management Environment(ACME)認証プロトコルを使用して、SSL証明書を自動的に生成および更新します（設定済みの場合）。詳細はこちらを参照してください：<br/><a href=" https://docs.openlitespeed.org/config/advanced/acme/ " target="_blank" rel="noopener noreferrer"> 自動SSL証明書（ACME） </a><br/>サーバーレベルで<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>に設定すると、この設定はサーバー全体で無効になります。それ以外の場合、サーバーレベル設定はバーチャルホストレベルで上書きできます。<br/>デフォルト値：<br/><b>サーバーレベル：</b>Off<br/><b>VHレベル：</b>サーバーレベル設定を継承', '', 'ドロップダウンリストから選択', '');

$_tipsdb['acme_dns_type'] = new DAttrHelp("ACME API dns_type", '(任意)ワイルドカード証明書生成API呼び出しを実行するときに使用するDNSタイプです。この値は使用するDNSプロバイダーによって異なり、ワイルドカード以外の証明書を生成する場合は不要です。使用可能なDNSタイプAPI値の詳細はこちらを参照してください:<br/><a href=" https://github.com/acmesh-official/acme.sh/wiki/dnsapi " target="_blank" rel="noopener noreferrer"> acme.sh-DNS APIの使用方法 </a><br/><br/>設定&quot;環境&quot;と組み合わせて使用します。', '', '', 'DNSプロバイダーがCloudFlareの場合、DNSタイプ値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>dns_cf</span>になります。');

$_tipsdb['acme_env'] = new DAttrHelp("環境", '(任意)ワイルドカード証明書生成API呼び出しを実行するときに含める環境値です。必要な具体的な値は使用するDNSプロバイダーによって異なり、ワイルドカード以外の証明書を生成する場合は不要です。必要な環境API値の詳細はこちらを参照してください:<br/><a href=" https://github.com/acmesh-official/acme.sh/wiki/dnsapi " target="_blank" rel="noopener noreferrer"> acme.sh-DNS APIの使用方法 </a><br/><br/>設定&quot;ACME API dns_type&quot;と組み合わせて使用します。', '', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Key=&quot;VALUE&quot;</span>ペアのリスト。1行に1つずつ入力し、各VALUEは二重引用符で囲みます。', 'DNSプロバイダーがCloudFlareの場合、必要な環境値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CF_Token=&quot;YOUR_TOKEN&quot;</span>のみです。');

$_tipsdb['acme_vhenable'] = new DAttrHelp("有効", 'Automatic Certificate Management Environment (ACME)証明書プロトコルを使用して、SSL証明書を自動的に生成および更新します(設定済みの場合)。詳細はこちらを参照してください:<br/><a href=" https://docs.openlitespeed.org/config/advanced/acme/ " target="_blank" rel="noopener noreferrer"> 自動SSL証明書（ACME） </a><br/><br/>サーバーレベルでこれを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>に設定すると、この設定はサーバー全体で無効になります。それ以外の場合、サーバーレベルの設定はバーチャルホストレベルで上書きできます。<br/><br/>デフォルト値:<br/><b>サーバーレベル:</b> Off<br/><b>VHレベル:</b> サーバーレベル設定を継承', '', 'ドロップダウンリストから選択', '');

$_tipsdb['addDefaultCharset'] = new DAttrHelp("デフォルトの文字セットを追加", 'コンテンツタイプが &quot;text/html&quot;または &quot;text/plain&quot;のいずれかのパラメータがない場合、文字セットタグを &quot;Content-Type&quot;レスポンスヘッダーに追加するかどうかを指定します。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Off</span>に設定すると、この機能は無効になります。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>On</span>に設定すると、&quot;カスタマイズされたデフォルトの文字セット&quot;で指定された文字セットまたはデフォルトの &quot;iso-8859-1&quot;のいずれかが追加されます。', '', 'ラジオボタンから選択', '');

$_tipsdb['addMIMEType'] = new DAttrHelp("MIMEタイプ", 'このコンテキストに追加のMIMEタイプとマッピングを指定します。 新しいマッピングは、このコンテキストとその子コンテキストのもとで既存のマッピングを上書きします。<br/>スクリプトとして実行するのではなく、PHPスクリプトをテキストファイルとして表示するには、MIMEタイプ &quot;text/plain&quot;への.phpマッピングをオーバーライドするだけです。', '', 'MIME-type1 extension extension ..., MIME-type2 extension ... 		カンマを使用してMIMEタイプを区切り、スペースを使用して複数の拡張子を区切ります。', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>image/jpg jpeg jpg, image/gif gif</span>');

$_tipsdb['addonmodules'] = new DAttrHelp("アドオンモジュール", '使用するアドオンモジュールを選択します。 ここに記載されていないバージョンを使用する場合は、手動でソースコードを更新することができます。 （PHPビルドのこのステップでは、ソースコードの場所がプロンプトに表示されます）。', '', 'チェックボックスから選択', '');

$_tipsdb['adminEmails'] = new DAttrHelp("管理者Eメール", 'サーバー管理者の電子メールアドレスを指定します。 このオプションを指定すると、重大イベントが電子メールで管理者に通知されます（例えば、LiteSpeedサービスがクラッシュを検出したために自動的に再開された場合、またはライセンスの期限切れになった時など）。', 'メールアラート機能は、サーバにpostfix、exim、sendmailなどのアクティブなMXサーバがある場合にのみ機能します。', 'Eメールアドレスのカンマ区切りリスト。', '');

$_tipsdb['adminNewPass'] = new DAttrHelp("新しいパスワード", 'このWebAdminユーザーの新しいパスワードを入力します。パスワードには任意の文字を使用でき、WebAdminユーザーの作成または更新時に必要です。', '', '', '');

$_tipsdb['adminOldPass'] = new DAttrHelp("旧パスワード", 'このWebAdminユーザーの現在のパスワードを入力します。ユーザー名またはパスワードの変更を保存する前に必要です。', '', '', '');

$_tipsdb['adminRetypePass'] = new DAttrHelp("パスワードを再入力してください", '新しいパスワードをもう一度入力します。新しいパスワードと一致する必要があります。', '', '', '');

$_tipsdb['adminUser'] = new DAttrHelp("WebAdminユーザー", 'WebAdminコンソールのユーザー名とパスワードを変更します。 変更を保存するには、古いパスワードを入力して確認する必要があります。', '', '', '');

$_tipsdb['adminUserName'] = new DAttrHelp("ユーザー名", 'WebAdminコンソールのログイン名を指定します。1～25文字で、英字、数字、ピリオド、アンダースコア、ハイフンのみ使用できます。', '', '', '');

$_tipsdb['allowBrowse'] = new DAttrHelp("アクセス可能", 'このコンテキストにアクセスできるかどうかを指定します。 アクセスを拒否するには<span class=&quot;lst-inline-token lst-inline-token--value&quot;>いいえ</span>に設定してください。 この機能を使用して、指定したディレクトリが訪問されないように保護できます。 このコンテキストのコンテンツを更新している場合、またはこのディレクトリに特別なデータがある場合に使用することができます。', '', 'ラジオボタンから選択', '');

$_tipsdb['allowQuic'] = new DAttrHelp("HTTP3/QUIC(UDP)ポートを開く", 'このリスナーにマップされたバーチャルホストでHTTP3/QUICネットワークプロトコルの使用を許可します。 この設定を有効にするには、サーバーレベルでも&quot;HTTP3/QUICを有効にする&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定する必要があります。 デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>です。', 'この設定を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定しても、バーチャルホストレベルの&quot;HTTP3/QUICを有効にする&quot;設定でHTTP3/QUICを無効にできます。', '', '');

$_tipsdb['allowSetUID'] = new DAttrHelp("UIDの設定を許可する", '設定されたUIDビットがCGIスクリプトに許可されるかどうかを指定します。 設定されたUIDビットが許可され、CGIスクリプトに対して設定されたUIDビットが有効になっている場合、CGIスクリプトがどのユーザーに代わって起動されたかにかかわらず、CGIプロセスのユーザーIDは、CGIスクリプトの所有者のユーザーIDに切り替わります。<br/>デフォルトは「オフ」です。', '[セキュリティ]可能であれば、UID CGIスクリプトを設定しないでください。 本質的にセキュリティリスクです。', 'ラジオボタンから選択', '');

$_tipsdb['allowSymbolLink'] = new DAttrHelp("シンボリックリンクをたどる", 'このバーチャルホスト内のシンボリックリンクをたどるかどうかを指定します。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>IF OWNER MATCH</span>オプションは、同じ所有権を持つ場合にのみシンボリックリンクに従います。 この設定は、デフォルトのサーバーレベルの設定よりも優先されます。', '[パフォーマンスとセキュリティ]セキュリティを強化するために、この機能を無効にしてください。 パフォーマンスを向上させるには、有効にします。', 'ドロップダウンリストから選択', '');

$_tipsdb['antiddos_blocked_ip'] = new DAttrHelp("Anti-DDoSブロック済みIP", 'Anti-DDoS保護によってブロックされたIPアドレスのカンマ区切りリストです。 各項目はセミコロンと、そのIPアドレスがブロックされた理由を示す理由コードで終わります。<br/><br/>使用可能な理由コード： <ul>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>A</span>: BOT_UNKNOWN</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>B</span>: BOT_OVER_SOFT</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>C</span>: BOT_OVER_HARD</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>D</span>: BOT_TOO_MANY_BAD_REQ</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>E</span>: BOT_CAPTCHA</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>F</span>: BOT_FLOOD</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>G</span>: BOT_REWRITE_RULE</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>H</span>: BOT_TOO_MANY_BAD_STATUS</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>I</span>: BOT_BRUTE_FORCE</li> </ul><br/><br/>ブロック済みIPの完全なリストは、&quot;統計出力ディレクトリ&quot;で設定されたリアルタイム統計レポートファイルでも確認できます。', '', '&lt;blocked_ip_address&gt;;&lt;reason_code&gt;', '1.0.100.50;E, 1.0.100.60;D, 1.0.100.70;F');

$_tipsdb['appServerContext'] = new DAttrHelp("App Serverコンテキスト", 'App Serverコンテキストは、Ruby Rack/Rails、WSGI、またはNode.jsアプリケーションを簡単に設定する方法を提供します。App Serverコンテキストを通じてアプリケーションを追加するには、URLとアプリケーションのルートディレクトリをマウントするだけで済みます。外部アプリケーションを定義し、404ハンドラを追加し、Rewriteルールを設定するなどの手間は不要です。', '', '', '');

$_tipsdb['appType'] = new DAttrHelp("アプリケーションタイプ", 'このコンテキストで使用するアプリケーションのタイプです。Rack/Rails、WSGI、またはNode.jsがサポートされています。', '', '', '');

$_tipsdb['appserverEnv'] = new DAttrHelp("実行時モード", 'アプリケーションを実行するモードを指定します。&quot;Development&quot;、&quot;Production&quot;、または&quot;Staging&quot;を選択できます。 デフォルトは&quot;Production&quot;です。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['as_location'] = new DAttrHelp("場所", 'ファイルシステム内で、このコンテキストに対応する場所を指定します。<br/><br/>デフォルト値：$DOC_ROOT + &quot;URI&quot;', '', '絶対パス、または$SERVER_ROOT、$VH_ROOT、$DOC_ROOTからの相対パスを指定できます。 $DOC_ROOTはデフォルトの相対パスであり、省略できます。<br/><br/>&quot;URI&quot;が正規表現の場合、一致した部分文字列を使用して&quot;Root&quot;文字列を構成できます。一致した部分文字列は&quot;$1&quot;から&quot;$9&quot;の値で参照できます。&quot;$0&quot;と&quot;&&quot;を使用して、一致した文字列全体を参照することもできます。さらに、&quot;?&quot;の後にクエリ文字列を追加して、クエリ文字列を設定できます。注意してください。クエリ文字列内の&quot;&&quot;は&quot;\&&quot;としてエスケープする必要があります。', '&quot;場所&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/john/web_examples</span>に設定した<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/examples/</span>のような通常のURIは、リクエスト&quot;/examples/foo/bar.html&quot;をファイル&quot;/home/john/web_examples/foo/bar.html&quot;にマップします。<br/>Apacheのmod_userdirをシミュレートするには、URIを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>exp: ^/~([A-Za-z0-9]+)(.*)</span>に設定し、&quot;場所&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/$1/public_html$2</span>に設定します。この設定では、URI/~john/foo/bar.htmlのリクエストはファイル/home/john/public_html/foo/bar.htmlにマップされます。');

$_tipsdb['as_startupfile'] = new DAttrHelp("起動ファイル", 'アプリケーションの起動に使用するファイルの場所です。アプリケーションルートディレクトリからの相対パスで指定します。<br/><br/>デフォルトの起動ファイル名には、Rack/Railsの&#039;config.ru&#039;、WSGIの&#039;wsgi.py&#039;と&#039;passenger_wsgi.py&#039;、NodeJSの&#039;app.js&#039;が含まれます。', '', 'パス', '');

$_tipsdb['authName'] = new DAttrHelp("認証名", '現在のコンテキストの認証レルムの代替名を指定します。 指定しない場合、元のレルム名が使用されます。 認証名は、ブラウザのログインポップアップに表示されます。', '', 'テキスト', '');

$_tipsdb['autoFix503'] = new DAttrHelp("503エラーの自動修復", 'サーバーをグレースフルリスタートすることで&quot;503 Service Unavailable&quot;エラーの修復を試みるかどうかを指定します。 &quot;503&quot;エラーは通常、外部アプリケーションの誤動作によって発生し、Webサーバーの再起動で一時的に修復できることがあります。 有効にすると、30秒以内に&quot;503&quot;エラーが30件を超えた場合、サーバーは自動的に再起動します。<br/><br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '', 'ラジオボックスから選択', '');

$_tipsdb['autoIndex'] = new DAttrHelp("自動インデックス", '&quot;インデックスファイル&quot;にリストされているインデックスファイルがディレクトリで使用できないときに、その場でディレクトリインデックスを生成するかどうかを指定します。 このオプションは、バーチャルホストおよびコンテキストレベルでカスタマイズ可能で、明示的に上書きされるまでディレクトリツリーに沿って継承されます。 生成された索引ページをカスタマイズできます。 オンラインWikiのHow-toを確認してください。', '[セキュリティ] 可能であれば、自動インデックスをオフにして、機密データの漏えいを防ぐことをお勧めします。', 'ラジオボックスから選択', '');

$_tipsdb['autoIndexURI'] = new DAttrHelp("自動インデックスURI", '&quot;インデックスファイル&quot;にリストされているインデックスファイルがディレクトリで使用できない場合に、インデックスページを生成するために使用されるURIを指定します。 LiteSpeed Webサーバーは外部スクリプトを使用してインデックスページを生成し、最大限のカスタマイズの柔軟性を提供します。 デフォルトスクリプトは、Apacheと同じ外観のインデックスページを生成します。 生成されたインデックスページをカスタマイズするには、オンラインWikiのHow-toを読んでください。 インデックス対象のディレクトリは、環境変数&quot;LS_AI_PATH&quot;を介してスクリプトに渡されます。', '', 'URI', '');

$_tipsdb['autoLoadHtaccess'] = new DAttrHelp(".htaccessから自動読み込み", 'そのディレクトリに最初にアクセスしたとき、<b>rewritefile</b>ディレクティブを使用するHttpContextがそのディレクトリにまだ存在しない場合、 ディレクトリの.htaccessファイルに含まれるRewriteルールを自動的に読み込みます。初回読み込み後にその.htaccessファイルへさらに変更を反映するには、 グレースフルリスタートを実行する必要があります。<br/><br/>バーチャルホストレベルの設定はサーバーレベルの設定を上書きします。デフォルト値：<br/><br/><b>サーバーレベル：</b>No<br/><br/><b>VHレベル：</b>サーバーレベル設定を継承', '', 'ラジオボックスから選択', '');

$_tipsdb['autoStart'] = new DAttrHelp("自動スタート", 'Webサーバーでアプリケーションを自動的に開始するかどうかを指定します。 同じマシン上で実行されているFastCGIおよびLSAPIアプリケーションのみを自動的に起動することができます。 &quot;アドレス&quot;のIPはローカルIPでなければなりません。 メインサーバプロセスではなくLiteSpeed CGIデーモンを起動すると、システムのオーバーヘッドを軽減できます。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['backlog'] = new DAttrHelp("バックログ", 'リスニングソケットのバックログを指定します。 &quot;自動スタート&quot;が有効な場合は必須です。', '', '整数', '');

$_tipsdb['banPeriod'] = new DAttrHelp("禁止期間（秒）", '&quot;猶予期間（秒）&quot;経過後、接続数がまだ&quot;接続ソフトリミット&quot;以上の場合、新しい接続がIPから拒否される期間を指定します。 IPが繰り返し禁止されている場合は、禁止期間を延長して虐待のペナルティを強化することをお勧めします。', '', '整数', '');

$_tipsdb['binPath'] = new DAttrHelp("バイナリパス", 'App Serverアプリケーションバイナリの場所です。', '', '', '');

$_tipsdb['blockBadReq'] = new DAttrHelp("不良リクエストブロック", '不正な形式のHTTPリクエストを&quot;禁止期間（秒）&quot;の間送信し続けるIPをブロックします。デフォルトは<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>です。 これは、不要なリクエストを繰り返し送信するボットネット攻撃のブロックに役立ちます。', '', 'ラジオボックスから選択', '');

$_tipsdb['brStaticCompressLevel'] = new DAttrHelp("Brotli圧縮レベル(静的ファイル)", '静的ファイルに適用するBrotli圧縮レベルを指定します。範囲は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>（無効）から<span class=&quot;lst-inline-token lst-inline-token--value&quot;>11</span>（最高）です。<br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定すると、Brotli圧縮はグローバルに無効になります。<br/>デフォルト値：5', '[パフォーマンス] ネットワーク帯域幅を節約します。html、css、javascriptファイルなどのテキストベースのレスポンスが最も効果を受け、平均で元のサイズの半分まで圧縮できます。', '0から11までの数値。', '');

$_tipsdb['bubbleWrap'] = new DAttrHelp("Bubblewrapコンテナ", 'CGIプロセス（PHPプログラムを含む）をbubblewrapサンドボックス内で起動する場合は、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Enabled</span>に設定します。bubblewrapの使用方法の詳細は次を参照してください。 <a href="   https://wiki.archlinux.org/title/Bubblewrap " target="_blank" rel="noopener noreferrer">   https://wiki.archlinux.org/title/Bubblewrap </a> この設定を使用する前に、システムにBubblewrapがインストールされている必要があります。<br/><br/>サーバーレベルで&quot;Disabled&quot;に設定されている場合、この設定をバーチャルホストレベルで有効にすることはできません。<br/><br/>デフォルト値：<br/><b>サーバーレベル：</b>Disabled<br/><b>VHレベル：</b>サーバーレベル設定を継承', '', 'ドロップダウンリストから選択', '');

$_tipsdb['bubbleWrapCmd'] = new DAttrHelp("Bubblewrapコマンド", 'bubblewrapプログラム自体を含む、完全なbubblewrap使用コマンドです。このコマンドの設定に関する詳細は次を参照してください。 <a href="   https://openlitespeed.org/kb/bubblewrap-in-openlitespeed/ " target="_blank" rel="noopener noreferrer">   https://openlitespeed.org/kb/bubblewrap-in-openlitespeed/ </a>。指定しない場合は、以下のデフォルトコマンドが使用されます。<br/><br/>デフォルト値： <span class=&quot;lst-inline-token lst-inline-token--command&quot;>/bin/bwrap --ro-bind /usr /usr --ro-bind /lib /lib --ro-bind-try /lib64 /lib64 --ro-bind /bin /bin --ro-bind /sbin /sbin --dir /var --dir /tmp --proc /proc --symlink ../tmp var/tmp --dev /dev --ro-bind-try /etc/localtime /etc/localtime --ro-bind-try /etc/ld.so.cache /etc/ld.so.cache --ro-bind-try /etc/resolv.conf /etc/resolv.conf --ro-bind-try /etc/ssl /etc/ssl --ro-bind-try /etc/pki /etc/pki --ro-bind-try /etc/man_db.conf /etc/man_db.conf --ro-bind-try /home/$USER /home/$USER --bind-try /var/lib/mysql/mysql.sock /var/lib/mysql/mysql.sock --bind-try /home/mysql/mysql.sock /home/mysql/mysql.sock --bind-try /tmp/mysql.sock /tmp/mysql.sock  --unshare-all --share-net --die-with-parent --dir /run/user/$UID ‘$PASSWD 65534’ ‘$GROUP 65534’</span>', '', '', '');

$_tipsdb['certChain'] = new DAttrHelp("チェーン証明書", '証明書がチェーン証明書であるかどうかを指定します。 チェーン証明を格納するファイルは、PEM形式でなければならず、証明書は最下位レベル（実際のクライアントまたはサーバー証明書）から最上位（ルート）CAまでの連鎖の順序でなければなりません。', '', 'ラジオボックスから選択', '');

$_tipsdb['certFile'] = new DAttrHelp("証明書ファイル", 'SSL証明書ファイルのファイル名。', '[セキュリティ]証明書ファイルは、サーバーが実行されるユーザーへの読み取り専用アクセスを可能にする安全なディレクトリに配置する必要があります。', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['cgiContext'] = new DAttrHelp("CGIコンテキスト", 'CGIコンテキストは、特定のディレクトリ内のスクリプトをCGIスクリプトとして定義します。 このディレクトリは、ドキュメントルートの内側または外側にすることができます。 このディレクトリの下にあるファイルが要求されると、サーバは実行可能かどうかに関わらず、常にCGIスクリプトとして実行しようとします。 このように、CGIコンテキスト下のファイルの内容は常に保護されており、静的なコンテンツとして読み込むことができません。 すべてのCGIスクリプトをディレクトリに置いて、それらにアクセスするためのCGIコンテキストを設定することをお勧めします。', '', '', '');

$_tipsdb['cgiResource'] = new DAttrHelp("CGI設定", '次の設定は、CGIプロセスを制御します。 これらのアプリケーションに対して制限が明示的に設定されていない場合は、メモリおよびプロセスの制限も他の外部アプリケーションのデフォルトとして機能します。', '', '', '');

$_tipsdb['cgi_path'] = new DAttrHelp("パス", 'CGIスクリプトの場所を指定します。', '', 'パスには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT/myapp/cgi-bin/</span>のようなCGIスクリプトのグループを含むディレクトリを指定できます。 この場合、コンテキスト&quot;URI&quot;は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/app1/cgi/</span>のように&quot;/&quot;で終わる必要があります。 パスには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT/myapp/myscript.pl</span>のようにCGIスクリプトを1つだけ指定することもできます。 このスクリプトには対応する&quot;URI&quot; <span class=&quot;lst-inline-token lst-inline-token--value&quot;>/myapp/myscript.pl</span>が必要です。', '');

$_tipsdb['cgidSock'] = new DAttrHelp("CGIデーモンソケット", 'CGIデーモンとの通信に使用される一意のソケットアドレス。 LiteSpeedサーバーは、スタンドアロンのCGIデーモンを使用して、最高のパフォーマンスとセキュリティを実現するCGIスクリプトを生成します。 デフォルトソケットは<span class=&quot;lst-inline-token lst-inline-token--value&quot;>uds://$SERVER_ROOT/admin/lscgid/.cgid.sock</span> &quot;です。 別の場所に配置する必要がある場合は、ここにUnixドメインソケットを指定します。', '', 'UDS://path', 'UDS://tmp/lshttpd/cgid.sock');

$_tipsdb['cgroups'] = new DAttrHelp("cgroups", 'プロセス群のリソース使用量（CPU、メモリ、ディスクI/O、ネットワークなど）を制限、集計、分離するLinuxカーネル機能です。ファイル<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/sys/fs/cgroup/cgroup.controllers</span>の存在で判定されるcgroups v2を実行している必要があります。<br/><br/>サーバーレベルで<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>に設定すると、この設定はサーバー全体で無効になります。それ以外の場合、サーバーレベルの設定はバーチャルホストレベルで上書きできます。<br/><br/>デフォルト値：<br/><b>サーバーレベル：</b>Off<br/><b>VHレベル：</b>サーバーレベル設定を継承', '', 'ドロップダウンリストから選択', '');

$_tipsdb['checkSymbolLink'] = new DAttrHelp("シンボリックリンクを確認する", '&quot;シンボリックリンクに従う&quot;がオンになっているときに、&quot;アクセスが拒否されたディレクトリ&quot;に対するシンボリックリンクをチェックするかどうかを指定します。 有効にすると、URLで参照されるリソースのカノニカル実パスが、設定可能なアクセス拒否ディレクトリと照合されます。 アクセスが拒否されたディレクトリ内にある場合、アクセスは拒否されます。', '[パフォーマンス & セキュリティ]最高のセキュリティを実現するには、このオプションを有効にします。 最高のパフォーマンスを得るには、無効にしてください。', 'ラジオボタンから選択', '');

$_tipsdb['ciphers'] = new DAttrHelp("暗号", 'SSLハンドシェイクのネゴシエーション時に使用する暗号スイートを指定します。 LSWSは、SSL v3.0、TLS v1.0、TLS v1.2、およびTLS v1.3で実装された暗号スイートをサポートしています。', '[セキュリティ] SSL暗号のベストプラクティスに従うデフォルト暗号を使用するため、このフィールドは空白のままにすることをお勧めします。', 'コロン区切りの暗号仕様文字列。', 'ECDHE-RSA-AES128-SHA256:RC4:HIGH:!MD5:!aNULL:!EDH');

$_tipsdb['clientVerify'] = new DAttrHelp("クライアントの検証", ' クライアント証明書認証のタイプを指定します。 使用できるタイプは次のとおりです： <ul> <li><b>None:</b> クライアント証明書は必要ありません。</li> <li><b>Optional:</b> クライアント証明書はオプションです。</li> <li><b>Require:</b> クライアントには有効な証明書が必要です。</li> <li><b>Optional_no_ca:</b> オプションと同じです。</li> </ul> デフォルトは &quot;None&quot;です。', '&quot;None&quot;または &quot;Require&quot;をお勧めします。', 'ドロップダウンリストから選択', '');

$_tipsdb['compilerflags'] = new DAttrHelp("コンパイラフラグ", '最適化されたコンパイラオプションのような追加のコンパイラフラグを追加します。', '', 'サポートされるフラグはCFLAGS、CXXFLAGS、CPPFLAGS、LDFLAGSです。異なるフラグはスペースで区切ります。フラグ値には二重引用符ではなく一重引用符を使用します。', 'CFLAGS=&#039;-O3 -msse2 -msse3 -msse4.1 -msse4.2 -msse4 -mavx&#039;');

$_tipsdb['compressibleTypes'] = new DAttrHelp("圧縮可能なタイプ", '圧縮を許可するMIMEタイプを指定します。この設定を未設定にするか<span class=&quot;lst-inline-token lst-inline-token--value&quot;>default</span>を入力すると、ほとんどのMIMEタイプをすでにカバーするサーバー組み込みのデフォルトリストを使用します。<br/>デフォルト値： <span class=&quot;lst-inline-token lst-inline-token--value&quot;>text/*,application/x-javascript,application/javascript,application/xml,image/svg+xml,application/rss+xml, application/json,application/vnd.ms-fontobject,application/x-font,application/x-font-opentype, application/x-font-truetype,application/x-font-ttf,font/eot,font/opentype,font/otf,font/ttf,image/x-icon, image/vnd.microsoft.icon,application/xhtml+xml</span>', '[パフォーマンス] GZIP/Brotli圧縮の効果があるタイプのみを許可してください。gif/png/jpeg画像やflashファイルなどのバイナリファイルは、圧縮の効果がありません。', 'MIMEタイプリストをカンマ区切りで指定します。text/*、!text/jsなど、ワイルドカード&quot;*&quot;と否定記号&quot;!&quot;を使用できます。', 'text/*を圧縮し、text/cssは圧縮しない場合、次のようなルールを設定できます。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>text/*, !text/css</span>。&quot;!&quot;はそのMIMEタイプを除外します。');

$_tipsdb['configFile'] = new DAttrHelp("設定ファイル", 'このバーチャルホストの設定ファイル名とディレクトリ。 設定ファイルは、$SERVER_ROOT/conf/vhosts/ディレクトリの下になければなりません。', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$SERVER_ROOT/conf/vhosts/$VH_NAME/vhconf.conf</span>をお勧めします。', 'ファイル名への絶対パス、または$SERVER_ROOTからの相対パス', '');

$_tipsdb['configureparams'] = new DAttrHelp("Configureパラメータ", 'PHPビルド用のconfigureパラメータを設定します。次のステップをクリックすると、Apache固有のパラメータと&quot;--prefix&quot;値は自動的に削除され、&quot;--with-litespeed&quot;が自動的に追加されます。 （プレフィックスは上のフィールドで設定できます。）これにより、既存の稼働中のPHPビルドのphpinfo()出力からconfigureパラメータをそのままコピーして貼り付けることができます。', '', 'スペース区切りの一連のオプション（二重引用符を使用する場合と使用しない場合）', '');

$_tipsdb['connTimeout'] = new DAttrHelp("コネクションタイムアウト（秒）", '1つの要求の処理中に許容される最大接続アイドル時間を指定します。 この時間の間、接続がアイドル状態の場合、つまりI/Oアクティビティがない場合は、接続が閉じられます。', '[セキュリティ]潜在的なDoS攻撃の間に不在接続を回復するのに役立つように、この値を低く設定します。', '整数', '');

$_tipsdb['consoleSessionTimeout'] = new DAttrHelp("セッションタイムアウト（秒）", 'WebAdminコンソールのセッションタイムアウト時間をカスタマイズします。 デフォルトは60秒です。', '[セキュリティ] 本番環境では適切な値を設定してください。通常は300秒未満です。', '整数', '');

$_tipsdb['cpuAffinity'] = new DAttrHelp("CPUアフィニティ", 'CPUアフィニティは、プロセスを1つ以上のCPU（コア）にバインドします。 プロセスが常に同じCPUを使用できると、CPUキャッシュに残ったデータを利用できるため有利です。 プロセスが別のCPUへ移動するとCPUキャッシュを利用できず、不要なオーバーヘッドが発生します。<br/><br/>CPUアフィニティ設定は、1つのサーバープロセスを関連付けるCPU（コア）の数を制御します。 最小値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>で、この機能を無効にします。最大値はサーバーのコア数です。 一般に<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>が最適です。CPUアフィニティを最も厳密に使用し、CPUキャッシュを最大限に活用できるためです。<br/><br/>デフォルト値：0', '', '0から64までの整数値。（0はこの機能を無効にします）', '');

$_tipsdb['crlFile'] = new DAttrHelp("クライアント失効ファイル", ' 取り消されたクライアント証明書を列挙するPEMエンコードCA CRLファイルを含むファイルを指定します。これは&quot;クライアントの失効パス&quot;の代替として、または追加で使用できます。', '', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['crlPath'] = new DAttrHelp("クライアントの失効パス", ' 取り消されたクライアント証明書のPEMエンコードされたCA CRLファイルを含むディレクトリを指定します。 このディレクトリのファイルはPEMでエンコードする必要があります。 これらのファイルは、ハッシュファイル名、hash-value.rNによってアクセスされます。 ハッシュファイル名の作成については、openSSLまたはApache mod_sslのドキュメントを参照してください。', '', 'パス', '');

$_tipsdb['ctxType'] = new DAttrHelp("コンテキストタイプ", '<b>静的</b>コンテキストを使用して、URIをドキュメントルートの外部またはその内部のディレクトリにマップできます。<br> <b>Java Webアプリ</b>コンテキストは、AJPv13準拠のJavaサーブレットエンジンで定義済みのJavaアプリケーションを自動的にインポートするために使用されます。<br> <b>サーブレット</b>コンテキストは、Webアプリケーションの下にある特定のサーブレットをインポートするために使用されます。<br> <b>Fast CGI</b>コンテキストは、Fast CGIアプリケーションのマウントポイントです。<br> <b>LiteSpeed SAPI</b>コンテキストを使用して、URIをLSAPIアプリケーションに関連付けることができます。<br> <b>プロキシー</b>コンテキストにより、このバーチャルホストは、外部のWebサーバーまたはアプリケーションサーバーへのトランスペアレントリバースプロキシサーバーとして機能します。<br> <b>CGI</b>コンテキストを使用して、ディレクトリにCGIスクリプトのみを指定することができます。<br> <b>ロードバランサー</b>コンテキストを使用して、そのコンテキストに異なるクラスタを割り当てることができます。<br> <b>リダイレクト</b>コンテキストで内部リダイレクトURIまたは外部リダイレクトURIを設定できます。<br> <b>Rack/Rails</b>コンテキストは、特にRack/Railsアプリケーションに使用されます。<br> <b>モジュールハンドラー</b>コンテキストは、ハンドラー型モジュールのマウントポイントです。<br>', '', '', '');

$_tipsdb['defaultCharsetCustomized'] = new DAttrHelp("カスタマイズされたデフォルトの文字セット", '&quot;デフォルトの文字セットを追加&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>On</span>のときに使用される文字セットを指定します。        これはオプションです。 デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;> iso-8859-1 </span>です。        &quot;デフォルトの文字セットを追加&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;> Off </span>の場合、このエントリは無効です。', '', '文字セットの名前。', 'utf-8');

$_tipsdb['defaultType'] = new DAttrHelp("デフォルトのMIMEタイプ", 'この型を指定すると、ドキュメントの接尾辞でMIMEタイプのマッピングを判別できない場合、または接尾辞がない場合にこの型が使用されます。        指定しない場合、デフォルト値<span class=&quot;lst-inline-token lst-inline-token--value&quot;>application/octet-stream</span>が使用されます。', '', 'MIMEタイプ', '');

$_tipsdb['destinationuri'] = new DAttrHelp("転送先URI", 'リダイレクト先の場所を指定します。 このリダイレクトされたURIが別のリダイレクトコンテキスト内のURIにマップされる場合、再度リダイレクトされます。', '', 'このURIは、&quot;/&quot;で始まる同じWebサイト上の相対URI、または&quot;http(s)://&quot;で始まる別のWebサイトを指す絶対URIのいずれかです。 &quot;URI&quot;に正規表現が含まれている場合、転送先は$1や$2などの一致した変数を参照できます。', '');

$_tipsdb['disableInitLogRotation'] = new DAttrHelp("初期ログローテーションを無効にする", '起動時にサーバーエラーログファイルのローテーションを有効/無効にするかどうかを指定します。 値が「未設定」の場合、初期ログローテーションはデフォルトで有効になっています。', '', 'ラジオボタンから選択', '');

$_tipsdb['docRoot'] = new DAttrHelp("ドキュメントルート", 'このバーチャルホストのドキュメントルートを指定します。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT/html</span>を推奨します。 このディレクトリは、コンテキストでは$DOC_ROOTとして参照されます。', '', '絶対パスか$SERVER_ROOTからの相対パス又は$VH_ROOTからの相対パス。', '');

$_tipsdb['domainName'] = new DAttrHelp("ドメイン", 'マッピングドメイン名を指定します。 ドメイン名は大文字と小文字を区別せず、先頭の&quot;www.&quot;は削除されます。 ワイルドカード文字&quot;*&quot;と&quot;?&quot;を使用できます。 &quot;?&quot;は1文字だけを表します。 &quot;*&quot;は任意の数の文字を表します。 重複したドメイン名は許可されません。', '[パフォーマンス] リスナーが1つのバーチャルホスト専用の場合は、不要なチェックを避けるために、ドメイン名に常に<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>を使用します。 可能であれば、catchallドメイン以外でワイルドカード文字を使用するドメイン名は避けてください。', 'カンマ区切りリスト。', 'www?.example.com<br/>&quot;*.mydomain.com&quot;はmydomain.comのすべてのサブドメインと一致します。<br/>&quot;*&quot;は単独でcatchallドメインとなり、一致しなかった任意のドメイン名に一致します。');

$_tipsdb['dynReqPerSec'] = new DAttrHelp("動的リクエスト/秒", '確立された接続の数に関係なく、1秒ごとに処理できる単一のIPアドレスからの動的に生成されるコンテンツへの要求の最大数を指定します。 この制限に達すると、今後のすべての動的コンテンツへのリクエストは、次の秒までタールピットされます。<br/><br/>静的コンテンツの要求制限は、この制限とは関係ありません。 このクライアントごとの要求制限は、サーバーまたはバーチャルホストレベルで設定できます。 バーチャルホストレベルの設定は、サーバーレベルの設定よりも優先されます。', '[セキュリティ]この制限によって、信頼できるIPまたはサブネットワークは制限されません。', '整数', '');

$_tipsdb['enableCoreDump'] = new DAttrHelp("コアダンプを有効にする", 'サーバーが&quot;root&quot;ユーザーによって起動されたときにコアダンプを有効にするかどうかを指定します。 最新のUnixシステムでは、セキュリティ上の理由から、ユーザーIDまたはグループIDを変更するプロセスはコアファイルをダンプできません。 ただし、コアダンプがあると問題の根本原因を特定しやすくなります。 このオプションは、Linuxカーネル2.4以上でのみ動作します。 Solarisユーザーは<span class=&quot;lst-inline-token lst-inline-token--command&quot;>coreadm</span>コマンドを使用してこの機能を制御してください。', '[セキュリティ] サーバーログファイルに<span class=&quot;lst-inline-token lst-inline-token--value&quot;>no core file created</span>と表示されている場合にのみ有効にします。 コアファイルを生成した直後に無効にしてください。 コアダンプが作成されたらバグレポートを提出してください。', 'ラジオボックスから選択', '');

$_tipsdb['enableDHE'] = new DAttrHelp("DHキー交換を有効にする", 'さらにSSL暗号化のためにDiffie-Hellman鍵交換を使用できます。', '[セキュリティ] DHキーの交換は、RSAキーを使用するよりも安全です。 ECDHとDHキーの交換は同等に安全です。<br/><br/>[パフォーマンス] DHキー交換を有効にするとCPU負荷が増加し、ECDHキー交換とRSAよりも遅くなります。 ECDH鍵交換が利用可能な場合は、そちらを優先することをお勧めします。', 'ラジオボックスから選択', '');

$_tipsdb['enableDynGzipCompress'] = new DAttrHelp("動的GZIP圧縮を有効にする", '動的に生成されるレスポンスのGZIP圧縮を制御します。<br/>この設定を有効にするには、&quot;圧縮を有効にする&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定する必要があります。<br/>デフォルト値：Yes', '[パフォーマンス] 動的レスポンスを圧縮するとCPUとメモリの使用量は増えますが、ネットワーク帯域幅を節約できます。', 'ラジオボックスから選択', '');

$_tipsdb['enableECDHE'] = new DAttrHelp("ECDH鍵交換を有効にする", 'さらにSSL暗号化のために楕円曲線 Diffie-Hellman鍵交換を使用できます。', '[セキュリティ] ECDH鍵交換は、RSA鍵だけを使用するより安全です。 ECDHとDHキーの交換は同等に安全です。<br/><br/>[パフォーマンス] ECDH鍵交換を有効にするとCPU負荷が増加し、RSA鍵だけを使用する場合よりも遅くなります。', 'ラジオボックスから選択', '');

$_tipsdb['enableExpires'] = new DAttrHelp("有効期限を有効にする", '静的ファイルのExpiresヘッダーを生成するかどうかを指定します。 有効にすると、&quot;デフォルトの期限&quot;と&quot;タイプ別の期限&quot;に基づいてExpiresヘッダーが生成されます。<br/><br/>これは、サーバー、バーチャルホスト、コンテキストレベルで設定できます。 下位レベルの設定は上位レベルの設定を上書きします。 コンテキスト設定はバーチャルホストの設定を上書きし、バーチャルホストの設定はサーバーの設定を上書きします。', '', 'ラジオボタンから選択', '');

$_tipsdb['enableGzipCompress'] = new DAttrHelp("圧縮を有効にする", '静的レスポンスと動的レスポンスの両方に対するGZIP/Brotli圧縮を有効にします。<br/>デフォルト値：Yes', '[パフォーマンス] ネットワーク帯域幅を節約するために有効にします。html、css、javascriptファイルなどのテキストベースのレスポンスが最も効果を受け、平均で元のサイズの半分まで圧縮できます。', 'ラジオボックスから選択', '');

$_tipsdb['enableIpGeo'] = new DAttrHelp("GeoLocationルックアップを有効にする", ' IPジオロケーション検索を有効または無効にするかどうかを指定します。 サーバー、バーチャルホスト、またはコンテキストレベルで設定できます。&quot;Not Set&quot;値を使用すると、IPジオロケーションはデフォルトで無効になります。', '', 'ラジオボックスから選択', '');

$_tipsdb['enableLVE'] = new DAttrHelp("CloudLinux", 'CloudLinuxのLightweight Virtual Environment(LVE)が存在する場合に有効にするかどうかを指定します。 LiteSpeedをLVEと併用すると、より優れたリソース管理を実現できます。 詳細については、 <a href="http://www.cloudlinux.com" target="_blank" rel="noopener noreferrer">http://www.cloudlinux.com</a> を参照してください。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['enableRecaptcha'] = new DAttrHelp("CAPTCHAを有効にする", '現在のレベルでCAPTCHA保護機能を有効にします。この機能を使用するには、サーバーレベルでこの設定を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定する必要があります。<br/><br/>デフォルト値：<br/><b>サーバーレベル：</b><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span><br/><b>VHレベル：</b>サーバーレベル設定を継承', '', 'ラジオボックスから選択', '');

$_tipsdb['enableRewrite'] = new DAttrHelp("Rewriteを有効にする", 'LiteSpeedのURL書き換えエンジンを有効にするかどうかを指定します。 このオプションは、バーチャルホストまたはコンテキストレベルでカスタマイズでき、明示的に上書きされるまでディレクトリツリーに沿って継承されます。', '', 'ラジオボックスから選択', '');

$_tipsdb['enableScript'] = new DAttrHelp("スクリプト/外部アプリを有効にする", 'このバーチャルホストでスクリプティング（非静的ページ）を許可するかどうかを指定します。 無効にすると、CGI、FastCGI、LSAPI、サーブレットエンジン、その他のスクリプト言語はこのバーチャルホストでは許可されません。 このため、スクリプトハンドラを使用する場合は、スクリプトハンドラもここで有効にする必要があります。', '', 'ラジオボックスから選択', '');

$_tipsdb['enableSpdy'] = new DAttrHelp("ALPN", 'Application-Layer Protocol Negotiation(ALPN)は、HTTP/3とHTTP/2ネットワークプロトコルを選択的に有効にするために使用されます。<br/><br/>HTTP/2とHTTP3を無効にする場合は、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>None</span>を選択し、他のすべてのチェックボックスを未選択のままにしてください。<br/><br/>デフォルト値: すべて有効', 'これはリスナーレベルとバーチャルホストレベルで設定できます。', 'チェックボックスから選択', '');

$_tipsdb['enableStapling'] = new DAttrHelp("OCSPステープルを有効にする", 'OCSPステープルを有効にするかどうかを決定します。これは、公開鍵証明書を検証するより効率的な方法です。', '', 'ラジオボックスから選択', '');

$_tipsdb['enableh2c'] = new DAttrHelp("平文TCP上のHTTP/2を有効にする", '暗号化されていないTCP接続に対してHTTP/2を有効にするかどうかを指定します。 デフォルトは無効です。', '', 'ラジオボタンから選択', '');

$_tipsdb['env'] = new DAttrHelp("環境", '外部アプリケーション用の追加の環境変数を指定します。', '', 'Key=value. 複数の変数は &quot;ENTER&quot;で区切ることができます。', '');

$_tipsdb['errCode'] = new DAttrHelp("エラーコード", 'エラーページのHTTPステータスコードを指定します。 選択したHTTPステータスコードだけがこのカスタマイズされたエラーページを持ちます。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['errPage'] = new DAttrHelp("カスタマイズされたエラーページ", 'サーバーが要求を処理する際に問題が発生すると、サーバーはエラーコードとhtmlページをエラーメッセージとしてWebクライアントに返します。 エラーコードはHTTPプロトコルで定義されています（RFC 2616参照）。 LiteSpeed Webサーバーには、エラーコードごとにデフォルトのエラーページが組み込まれていますが、各エラーコードに対してカスタマイズされたページを設定することもできます。 これらのエラーページは、各バーチャルホストごとに一意になるようにさらにカスタマイズすることができます。', '', '', '');

$_tipsdb['errURL'] = new DAttrHelp("URL", 'カスタマイズされたエラーページのURLを指定します。 サーバーは、対応するHTTPステータスコードが返されたときに、このURLにリクエストを転送します。 このURLが存在しないリソースを参照する場合は、組み込みのエラーページが使用されます。 URLは、静的ファイル、動的に生成されたページ、または別のWebサイトのページ（&quot;http(s)://&quot;で始まるURL）にできます。 別のWebサイトのページを参照する場合、クライアントは元のステータスコードの代わりにリダイレクトステータスコードを受け取ります。', '', 'URL', '');

$_tipsdb['expWSAddress'] = new DAttrHelp("アドレス", '外部Webサーバーによって使用されるHTTPまたはHTTPSアドレス。', '[セキュリティ]同じマシン上で実行されている別のWebサーバーにプロキシする場合は、IPアドレスを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>localhost</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>127.0.0.1</span>に設定します。 外部アプリケーションは他のマシンからはアクセスできません。', 'IPv4またはIPv6アドレス（:ポート）。 外部WebサーバーがHTTPSを使用する場合は、先頭に「https://」を追加します。 外部Webサーバーが標準ポート80または443を使用する場合、ポートはオプションです。', '192.168.0.10<br/>127.0.0.1:5434<br/>https://10.0.8.9<br/>https://127.0.0.1:5438');

$_tipsdb['expiresByType'] = new DAttrHelp("タイプ別の期限", '各MIMEタイプのExpiresヘッダー設定を指定します。', '', '&quot;MIME-type=A|Mseconds&quot;のカンマで区切られたリスト。 このファイルは、基本時間（A|M）に指定された秒を加えた後に期限切れになります。<br/><br/>ベース時刻 &quot;A&quot;はクライアントのアクセス時間に値を設定し、 &quot;M&quot;はファイルの最終変更時刻を設定します。 MIMEタイプはimage/*のようなワイルドカード &quot;*&quot;を受け入れます。', '');

$_tipsdb['expiresDefault'] = new DAttrHelp("デフォルトの期限", 'Expiresヘッダー生成のデフォルト設定を指定します。 この設定は、&quot;有効期限を有効にする&quot;が &quot;はい&quot;に設定されているときに有効になります。 &quot;タイプ別の期限&quot;で上書きできます。 すべてのページのExpiresヘッダーが生成されるため、必要がない限り、このデフォルトをサーバーまたはバーチャルホストレベルで設定しないでください。 ほとんどの場合、これは頻繁に変更されない特定のディレクトリのコンテキストレベルで設定する必要があります。 デフォルト設定がない場合、&quot;タイプ別の期限&quot;で指定されていないタイプに対してExpiresヘッダーは生成されません。', '', 'A|M秒<br/>このファイルは、基本時間（A | M）に指定された秒を加えた後に期限切れになります。 ベース時刻 &quot;A&quot;はクライアントのアクセス時間に値を設定し、 &quot;M&quot;はファイルの最終変更時刻を設定します。', '');

$_tipsdb['expuri'] = new DAttrHelp("URI", 'このコンテキストのURIを指定します。', '', 'URIは通常のURI（&quot;/&quot;で始まる）またはPerl互換の正規表現URI（&quot;exp:&quot;で始まる）にできます。通常のURIが&quot;/&quot;で終わる場合、このコンテキストはこのURI配下のすべてのサブURIを含みます。コンテキストがファイルシステム上のディレクトリにマップされる場合は、末尾に&quot;/&quot;を追加する必要があります。', '');

$_tipsdb['extAppAddress'] = new DAttrHelp("アドレス", '外部アプリケーションによって使用される一意のソケットアドレス。 IPv4/IPv6ソケットとUnixドメインソケット（UDS）がサポートされています。 IPv4/IPv6ソケットは、ネットワークを介した通信に使用できます。 UDSは、外部アプリケーションがサーバーと同じマシンにある場合にのみ使用できます。', '[セキュリティ]外部アプリケーションが同じマシン上で実行される場合は、UDSが優先されます。 IPv4/IPv6ソケットを使用する必要がある場合は、IPアドレスを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>localhost</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>127.0.0.1</span>に設定して、外部アプリケーションに他のマシンからアクセスできないようにします。<br/>[パフォーマンス] Unixドメインソケットは、通常、IPv4ソケットよりも高いパフォーマンスを提供します。', 'IPv4またはIPv6アドレス:ポート、またはUDS://path', '127.0.0.1:5434<br/>UDS://tmp/lshttpd/php.sock.');

$_tipsdb['extAppName'] = new DAttrHelp("名前", 'この外部アプリケーションの一意の名前。 設定の他の部分でこの名前を使用するときは、この名前で参照します。', '', 'テキスト', '');

$_tipsdb['extAppPath'] = new DAttrHelp("コマンド", '外部アプリケーションを実行するためのパラメータを含む完全なコマンドラインを指定します。 &quot;自動スタート&quot;が有効な場合は必須値です。 パラメータにスペースまたはタブ文字が含まれている場合は、パラメータを二重引用符または一重引用符で囲む必要があります。', '', 'オプションのパラメータを含む実行可能ファイルへのフルパス。', '');

$_tipsdb['extAppPriority'] = new DAttrHelp("優先度", '外部アプリケーションプロセスの優先度を指定します。値の範囲は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-20</span>から<span class=&quot;lst-inline-token lst-inline-token--value&quot;>20</span>です。 数字が小さいほど優先度は高くなります。外部アプリケーションプロセスはWebサーバーより高い優先度を持つことはできません。 この優先度がサーバーより小さい数値に設定されている場合、この値にはサーバーの優先度が使用されます。', '', '整数', '');

$_tipsdb['extAppType'] = new DAttrHelp("タイプ", '外部アプリケーションのタイプを指定します。 アプリケーションタイプは、提供するサービスまたはサーバーとの通信に使用するプロトコルによって区別されます。 以下から選んでください。 <ul> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>FastCGI</span>: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Responder</span>ロールを持つFastCGIアプリケーションです。</li> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>FastCGI Authorizer</span>: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Authorizer</span>ロールを持つFastCGIアプリケーション</li> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Servlet Engine</span>: tomcatなどのAJPv13コネクタを持つサーブレットエンジンです。</li> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Web Server</span>: HTTPプロトコルをサポートするWebサーバーまたはアプリケーションサーバー。</li> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>LiteSpeed SAPI App</span>: LSAPIプロトコルを使用してWebサーバーと通信するアプリケーションです。</li> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Load Balancer</span>:ワーカーアプリケーション間の負荷を分散できるバーチャルアプリケーションです。</li> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Piped Logger</span>: STDINストリームで受け取ったアクセスログエントリを処理できるアプリケーションです。</li> </ul>', 'ほとんどのアプリケーションは、LSAPIまたはFastCGIプロトコルを使用します。 LSAPIはPHP、Ruby、Pythonをサポートしています。 PerlはFastCGIで使用できます。 （PHP、Ruby、およびPythonはFastCGIを使用して実行するように設定することもできますが、LSAPIを使用する方が高速です）。Javaはサーブレットエンジンを使用します。', 'ドロップダウンリストから選択', '');

$_tipsdb['extAuthorizer'] = new DAttrHelp("承認者", '権限のある/権限のないかの決定を生成するために使用できる外部アプリケーションを指定します。 現在、FastCGI Authorizerのみが使用可能です。 FastCGI Authorizerの役割の詳細については、<a href="   https://fastcgi-archives.github.io/ " target="_blank" rel="noopener noreferrer">   https://fastcgi-archives.github.io/ </a>を参照してください。 。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['extGroup'] = new DAttrHelp("実行グループ", '外部アプリケーションを実行するグループ名を指定します。 未設定の場合は、バーチャルホストレベルの設定を継承します。<br/><br/>デフォルト値：未設定', '', '有効なグループ名。', '');

$_tipsdb['extMaxIdleTime'] = new DAttrHelp("最大アイドル時間", '外部アプリケーションがサーバーによって停止され、アイドルリソースが解放されるまでの最大アイドル時間を指定します。 &quot;-1&quot;に設定すると、ProcessGroupモードで実行されている場合を除き、外部アプリケーションはサーバーによって停止されません。 ProcessGroupモードでは、アイドル状態の外部アプリケーションは30秒後に停止されます。<br/><br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-1</span>', '[パフォーマンス] この機能は大量ホスティング環境で特に便利です。 あるバーチャルホストが所有するファイルに、別のバーチャルホストの外部アプリケーションスクリプトからアクセスされるのを防ぐため、 SetUIDモードで多数の異なるアプリケーションを同時に実行する必要があります。 これらの外部アプリケーションが不要にアイドル状態のままにならないよう、この値を低く設定してください。', '整数', '');

$_tipsdb['extUmask'] = new DAttrHelp("umask", 'この外部アプリケーションのプロセスのデフォルトumaskを設定します。 詳細は、<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 umask</span>を参照してください。 デフォルト値はサーバーレベルの&quot;umask&quot;設定から取得されます。', '', '有効範囲[000]〜[777]の値。', '');

$_tipsdb['extUser'] = new DAttrHelp("実行ユーザー", '外部アプリケーションを実行するユーザー名を指定します。 未設定の場合は、バーチャルホストレベルの設定を継承します。<br/><br/>デフォルト値：未設定', '', '有効なユーザー名。', '');

$_tipsdb['extWorkers'] = new DAttrHelp("ワーカー", '以前に外部ロードバランサで定義されたワーカーグループのリスト。', '', 'ExternalAppType::ExternalAppNameという形式のカンマ区切りリスト', 'fcgi::localPHP, proxy::backend1');

$_tipsdb['externalredirect'] = new DAttrHelp("外部リダイレクト", 'このリダイレクトが外部かどうかを指定します。 外部リダイレクトの場合は、&quot;ステータスコード&quot;を指定し、&quot;転送先URI&quot;は &quot;/&quot;または &quot;http(s)：//&quot;で開始できます。 内部リダイレクトの場合、&quot;転送先URI&quot;は &quot;/&quot;で始まらなければなりません。', '', '', '');

$_tipsdb['extraHeaders'] = new DAttrHelp("ヘッダー操作", '追加するレスポンス/リクエストヘッダーを指定します。複数のヘッダーディレクティブを1行に1つ追加できます。&quot;NONE&quot;を使用すると、親ヘッダーの継承を無効にできます。ディレクティブが指定されていない場合は&#039;Header&#039;と見なされます。', ' 構文と使用方法は、サポートされる操作について <a href="https://httpd.apache.org/docs/2.2/mod/mod_headers.html#header" target="_blank" rel="noopener noreferrer">Apacheのmod_headersディレクティブ</a> に似ています。<br/><br/> &#039;Header&#039;ディレクティブは任意であり、他の場所からルールをコピーする際に除外しても残しても問題ありません。', '[Header]|RequestHeader [condition] set|append|merge|add|unset header [value] [early|env=[!]variable]', 'set Cache-control no-cache<br/>append Cache-control no-store<br/>Header set My-header cust_header_val<br/>RequestHeader set My-req-header cust_req_header_val');

$_tipsdb['extrapathenv'] = new DAttrHelp("追加のPATH環境変数", 'ビルドスクリプト用に、現在のPATH環境変数へ追加されるPATH値です。', '', '“:”で区切られたパス値', '');

$_tipsdb['fcgiContext'] = new DAttrHelp("FastCGIコンテキスト", 'FastCGIアプリケーションは直接使用することはできません。 FastCGIアプリケーションは、スクリプトハンドラとして構成するか、FastCGIコンテキストを介してURLにマップする必要があります。 FastCGIコンテキストは、URIをFastCGIアプリケーションに関連付けます。', '', '', '');

$_tipsdb['fcgiapp'] = new DAttrHelp("FastCGI アプリ", 'FastCGIアプリケーションの名前を指定します。 このアプリケーションは、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['fileETag'] = new DAttrHelp("ファイルETag", 'ファイルのinode、last-modified time、およびsize属性を使用するかどうかを指定します。 静的ファイル用のETag HTTP応答ヘッダーを生成します。 3つの属性はすべてデフォルトで有効になっています。 ミラー化されたサーバーから同じファイルを提供する予定の場合は、iノードを含めないでください。 それ以外の場合、1つのファイルに対して生成されるETagは、異なるサーバーで異なります。', '', 'チェックボックスから選択', '');

$_tipsdb['fileUpload'] = new DAttrHelp("ファイルアップロード", 'Request Body Parserを使用してファイルをアップロードしてサーバーのローカルディレクトリにファイルを解析し、第三者のモジュールによる悪意のある行為を簡単にスキャンできるようにする、追加のセキュリティ機能を提供します。 Request Body Parserは、&quot;ファイルパスによるアップロードデータの転送&quot;が有効になっているか、モジュールがLSI_HKPT_HTTP_BEGINレベルでLSIAPIのset_parse_req_bodyを呼び出すときに使用されます。 ソースパッケージで提供されるAPIの例', '', '', '');

$_tipsdb['followSymbolLink'] = new DAttrHelp("シンボリックリンクに従う", '静的ファイルを提供する際のシンボリックリンクのサーバーレベルのデフォルト設定を指定します。<br/><br/>選択肢は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>オーナーと一致する場合</span> <span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>です。<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>は、シンボリックリンクに常に従うようにサーバーを設定します。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>所有者の一致がの場合</span>は、リンクの所有者とターゲットの所有者が同じ場合にのみシンボリックリンクに従うようにサーバーを設定します。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>は、サーバーがシンボリックリンクを決して辿らないことを意味します。 この設定はバーチャルホスト設定で上書きできますが、.htaccessファイルで上書きすることはできません。', '[パフォーマンスとセキュリティ]最高のセキュリティを実現するには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>If Owner Match</span>を選択します。 最高のパフォーマンスを得るには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>を選択します。', 'ドロップダウンリストから選択', '');

$_tipsdb['forceGID'] = new DAttrHelp("強制GID", 'suEXECモードで起動したすべての外部アプリケーションに使用するグループIDを指定します。 ゼロ以外の値に設定すると、すべてのsuEXEC外部アプリケーション（CGI/FastCGI/LSAPI）がこのグループIDを使用します。 これにより、外部アプリケーションが他のユーザーが所有するファイルにアクセスするのを防ぐことができます。<br/><br/>たとえば、共有ホスティング環境では、LiteSpeedはユーザー &quot;www-data&quot;として実行され、グループ &quot;www-data&quot;として実行されます。 各docrootは、 &quot;www-data&quot;のグループと許可モード0750を持つユーザーアカウントによって所有されています。 強制GIDが &quot;nogroup&quot;（または &#039;www-data&#039;以外のグループ）に設定されている場合、すべてのsuEXEC外部アプリケーションは特定のユーザーとして実行されますが、グループ &quot;nogroup&quot;に実行されます。 これらの外部アプリケーション・プロセスは（ユーザーIDのために）特定のユーザーが所有するファイルには引き続きアクセスできますが、他のユーザーのファイルにアクセスするためのグループ権限は持っていません。 一方、サーバは、（グループIDのために）どんなユーザのdocrootディレクトリ下でもファイルを提供することができます。', '[セキュリティ]システムユーザーが使用するすべてのグループを除外するのに十分な高さに設定します。', '整数', '');

$_tipsdb['forceStrictOwnership'] = new DAttrHelp("厳格な所有権チェックを強制する", '厳密なファイル所有権チェックを実施するかどうかを指定します。 有効になっている場合、Webサーバーは、提供されるファイルの所有者がバーチャルホストの所有者と同じかどうかをチェックします。 異なる場合は、403アクセス拒否エラーが返されます。 これはデフォルトではオフになっています。', '[セキュリティ]共有ホスティングの場合、このチェックを有効にしてセキュリティを強化します。', 'ラジオボタンから選択', '');

$_tipsdb['forceType'] = new DAttrHelp("強制MIMEタイプ", 'このコンテキストの下にあるすべてのファイルは、指定すると、ファイルの接尾辞に関係なくMIMEタイプが指定された静的ファイルとして提供されます。        <span class=&quot;lst-inline-token lst-inline-token--value&quot;> NONE </span>に設定すると、継承された強制タイプの設定は無効になります。', '', 'MIMEタイプまたはNONE。', '');

$_tipsdb['gdb_groupname'] = new DAttrHelp("グループ名", '英字と数字のみを含むグループ名です（特殊文字は使用できません）。', '', '文字列', '');

$_tipsdb['gdb_users'] = new DAttrHelp("ユーザー", 'このグループに属するユーザーのスペース区切りリストです。', '', '', '');

$_tipsdb['generalContext'] = new DAttrHelp("静的コンテキスト", 'コンテキスト設定は、特定の場所にあるファイルの特別な設定を指定するために使用されます。 これらの設定は、ApacheのAliasやAliasMatchディレクティブのようなドキュメントルートの外部にファイルを持ち込み、認可領域を使用して特定のディレクトリを保護したり、ドキュメントルート内の特定のディレクトリへのアクセスをブロックまたは制限するために使用できます。', '', '', '');

$_tipsdb['geoipDBFile'] = new DAttrHelp("DBファイルパス", 'MaxMind GeoIPデータベースへのパスを指定します。', '', '絶対パス', '/usr/local/share/GeoIP/GeoLite2-Country.mmdb');

$_tipsdb['geoipDBName'] = new DAttrHelp("DB名", 'MaxMind GeoIPデータベース名です。GeoIP2以降、この設定は必須です。<br/><br/>GeoIPからGeoIP2へアップグレードする場合、この設定に&quot;COUNTRY_DB&quot;、&quot;CITY_DB&quot;、または&quot;ASN_DB&quot;を使用すると、 移行を容易にするため、PHPの$_SERVER変数にGeoIP互換エントリ（以下のDB名値別一覧）が自動的に設定されます。<br/><br/><b>CITY_DB:</b> &#039;GEOIP_COUNTRY_CODE&#039;, &#039;GEOIP_COUNTRY_NAME&#039;, &#039;GEOIP_CONTINENT_CODE&#039;, &#039;GEOIP_COUNTRY_CONTINENT&#039;, &#039;GEOIP_DMA_CODE&#039;, &#039;GEOIP_METRO_CODE&#039;, &#039;GEOIP_LATITUDE&#039;, &#039;GEOIP_LONGITUDE&#039;, &#039;GEOIP_POSTAL_CODE&#039;, and &#039;GEOIP_CITY&#039;.<br/><b>COUNTRY_DB:</b> &#039;GEOIP_COUNTRY_CODE&#039;, &#039;GEOIP_COUNTRY_NAME&#039;, &#039;GEOIP_CONTINENT_CODE&#039;, and &#039;GEOIP_COUNTRY_CONTINENT&#039;.<br/><b>ASN_DB:</b> &#039;GEOIP_ORGANIZATION&#039; and &#039;GEOIP_ISP&#039;.', '', '', 'COUNTRY_DB');

$_tipsdb['geolocationDB'] = new DAttrHelp("MaxMind GeoIP DB", '複数のMaxMindジオロケーションデータベースをここで指定できます。MaxMindには、 Country、Region、City、Organization、ISP、NetspeedタイプのDBがあります。 &quot;Country&quot;、&quot;Region&quot;、&quot;City&quot;タイプの複数のデータベースが設定されている場合、最後の設定が有効になります。', '', '', '');

$_tipsdb['gracePeriod'] = new DAttrHelp("猶予期間（秒）", '1つのIPから確立された接続の数が&quot;接続ソフトリミット&quot;を超えた後に新しい接続が受け入れられる期間を指定します。 この期間内に、総接続数が&quot;接続ハードリミット&quot;未満の場合は、新しい接続が受け入れられます。 この期間が経過した後、まだ接続数が&quot;接続ソフトリミット&quot;よりも高い場合、問題のIPは&quot;禁止期間（秒）&quot;でブロックされます。', '[パフォーマンス & セキュリティ]ダウンロードに十分な大きさに設定してください。 完全なページですが、故意の攻撃を防ぐのに十分な低さです。', '整数', '');

$_tipsdb['gracefulRestartTimeout'] = new DAttrHelp("グレースフルリスタートタイムアウト（秒）", 'グレースフルリスタート時には、新しいサーバーインスタンスが起動した後でも、古いインスタンスは既存の要求を引き続き処理します。 このタイムアウトは、前のインスタンスが終了するまでの待機時間を定義します。 デフォルト値は300秒です。 -1は永遠に待つことを意味します。 0は待機しないことを意味し、直ちに中止します。', '', '整数', '');

$_tipsdb['groupDBCacheTimeout'] = new DAttrHelp("グループDBキャッシュタイムアウト（秒）", 'バックエンドグループデータベースの変更の確認頻度を指定します。 詳細については、&quot;ユーザーDBキャッシュタイムアウト（秒）&quot;を参照してください。', '', '整数', '');

$_tipsdb['groupDBMaxCacheSize'] = new DAttrHelp("グループDB最大キャッシュサイズ", 'グループデータベースの最大キャッシュサイズを指定します。', '[パフォーマンス]キャッシュが大きくなるとメモリが消費されるため、値が高くなるほどパフォーマンスが向上する場合があります。 ユーザーのデータベースサイズとサイトの使用状況に応じて適切なサイズに設定します。', '整数', '');

$_tipsdb['gzipAutoUpdateStatic'] = new DAttrHelp("静的ファイルの自動更新", '圧縮可能な静的ファイルについて、サーバーがGZIP圧縮版を自動的に作成/更新するかどうかを指定します。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定されている場合、&quot;圧縮可能なタイプ&quot;にリストされたMIMEタイプのファイルが要求されると、サーバーは圧縮ファイルのタイムスタンプに応じて、そのファイルに対応する圧縮版を作成または更新できます。この圧縮ファイルは&quot;静的キャッシュディレクトリ&quot;の下に作成されます。ファイル名は元のファイルパスのMD5ハッシュに基づきます。<br/>デフォルト値：Yes', '', 'ラジオボックスから選択', '');

$_tipsdb['gzipCacheDir'] = new DAttrHelp("静的キャッシュディレクトリ", '静的コンテンツ用の圧縮ファイルを保存するために使用するディレクトリのパスを指定します。<br/>デフォルト値：&quot;スワッピングディレクトリ&quot;。', '', 'ディレクトリパス', '');

$_tipsdb['gzipCompressLevel'] = new DAttrHelp("GZIP圧縮レベル(動的コンテンツ)", '動的コンテンツに適用するGZIP圧縮レベルを指定します。範囲は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>（最低）から<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>（最高）です。<br/>この設定は、&quot;圧縮を有効にする&quot;と&quot;動的GZIP圧縮を有効にする&quot;が有効な場合にのみ有効です。<br/>デフォルト値：6', '[パフォーマンス] 圧縮レベルを高くすると、より多くのメモリとCPUサイクルを使用します。マシンに余力がある場合は、より高いレベルに設定できます。6と9の間に大きな差はありませんが、9ではCPUサイクルをかなり多く使用します。', '1から9までの数値。', '');

$_tipsdb['gzipMaxFileSize'] = new DAttrHelp("最大静的ファイルサイズ(バイト)", 'サーバーが自動的に圧縮ファイルを作成する静的ファイルの最大サイズを指定します。<br/>デフォルト値：10M', '[パフォーマンス] 大きなファイルに対してサーバーが圧縮ファイルを作成/更新することは推奨されません。圧縮はサーバープロセス全体をブロックし、圧縮が完了するまで以降のリクエストを処理できません。', '1K以上のバイト数。', '');

$_tipsdb['gzipMinFileSize'] = new DAttrHelp("最小静的ファイルサイズ(バイト)", 'サーバーが対応する圧縮ファイルを作成する静的ファイルの最小サイズを指定します。<br/>デフォルト値：200', '帯域幅の節約はごくわずかなため、非常に小さいファイルを圧縮する必要はありません。', '200以上のバイト数。', '');

$_tipsdb['gzipStaticCompressLevel'] = new DAttrHelp("GZIP圧縮レベル(静的ファイル)", '静的ファイルに適用するGZIP圧縮レベルを指定します。範囲は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>（最低）から<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>（最高）です。<br/>この設定は、&quot;圧縮を有効にする&quot;と&quot;静的ファイルの自動更新&quot;が有効な場合にのみ有効です。<br/>デフォルト値：6', '', '1から9までの数値。', '');

$_tipsdb['hardLimit'] = new DAttrHelp("接続ハードリミット", '単一のIPアドレスから同時に許可される接続の最大数を指定します。 この制限は常に強制され、クライアントはこの制限を超えることはできません。 HTTP/1.0クライアントは通常、埋め込みコンテンツを同時にダウンロードするのに必要な数の接続を設定しようとします。 この制限は、HTTP/1.0クライアントが引き続きサイトにアクセスできるように十分に高く設定する必要があります。 &quot;接続ソフトリミット&quot;を使用して、目的の接続制限を設定します。', '[セキュリティ]数字が小さいほど、より明確なクライアントに対応できます。<br/>[セキュリティ]信頼できるIPまたはサブネットワークは影響を受けません。<br/>[パフォーマンス]多数の同時クライアントマシンでベンチマークテストを実行する場合は、高い値に設定します。', '整数', '');

$_tipsdb['httpdWorkers'] = new DAttrHelp("ワーカーの数", 'httpdワーカーの数を指定します。', '[パフォーマンス]ニーズに合わせて適切な番号を設定します。 より多くのワーカーを追加することは、必ずしもより良いパフォーマンスを意味するとは限りません。', '整数値は1〜16です。', '');

$_tipsdb['inBandwidth'] = new DAttrHelp("受信帯域幅（バイト/秒）", '確立された接続の数に関係なく、単一のIPアドレスからの最大許容着信スループット。 実際の帯域幅は効率上の理由からこの設定よりわずかに高くなることがあります。 帯域幅は1KB単位で割り当てられます。 スロットルを無効にするには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定します。 クライアント単位の帯域幅制限（バイト/秒）は、バーチャルホストレベルの設定がサーバーレベルの設定を上回るサーバーまたはバーチャルホストレベルで設定できます。', '[セキュリティ]信頼できるIPまたはサブネットワークは影響を受けません。', '整数', '');

$_tipsdb['inMemBufSize'] = new DAttrHelp("メモリI/Oバッファサイズ", '要求本体およびその動的生成応答を格納するために使用される最大バッファー・サイズを指定します。 この制限に達すると、サーバーは&quot;スワッピングディレクトリ&quot;の下に一時的なスワップファイルを作成し始めます。', '[パフォーマンス]メモリとディスクのスワップを避けるために、すべての同時要求/応答を収容できる大きさのバッファサイズを設定します。 スワップディレクトリに頻繁にI/Oアクティビティがある場合（デフォルトでは/tmp/lshttpd/swap/）、このバッファサイズは小さすぎるため、LiteSpeedはディスクにスワップします。', '整数', '');

$_tipsdb['indexFiles'] = new DAttrHelp("インデックスファイル", 'URLがディレクトリにマップされたときに順番に検索されるインデックスファイルの名前を指定します。 サーバー、バーチャルホスト、コンテキストレベルでカスタマイズできます。', '[パフォーマンス]必要なインデックスファイルのみを設定します。', 'インデックスファイル名のカンマ区切りリスト。', '');

$_tipsdb['indexUseServer'] = new DAttrHelp("サーバーインデックスファイルを使用する", 'サーバーのインデックスファイル設定を使用するかどうかを指定します。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定すると、サーバーの設定だけが使用されます。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>に設定すると、サーバーの設定は使用されません。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Addition</span>に設定すると、このバーチャルホストのサーバーインデックスファイルリストに追加のインデックスファイルを追加できます。 このバーチャルホストのインデックスファイルを無効にする場合は、値を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>に設定して、インデックスファイル欄を空のままにします。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['initTimeout'] = new DAttrHelp("初期リクエストタイムアウト（秒）", '新しく確立された接続で、外部アプリケーションが最初のリクエストに応答するまでサーバーが待機する最大秒数を指定します。 このタイムアウト内に外部アプリケーションからデータを受信しない場合、サーバーはこの接続を不良としてマークします。 これにより、外部アプリケーションとの通信問題をできるだけ早く特定できます。 処理に時間がかかるリクエストがある場合は、503エラーメッセージを避けるためにこの制限を増やしてください。', '', '整数', '');

$_tipsdb['installpathprefix'] = new DAttrHelp("インストールパスのプレフィックス", '&quot;--prefix&quot;設定オプションの値を設定します。デフォルトのインストール先はLiteSpeed Web Serverのインストールディレクトリ配下です。', 'LiteSpeed Web Serverは、複数のPHPバージョンを同時に使用できます。複数のバージョンをインストールする場合は、それぞれ異なるプレフィックスを指定してください。', 'パス', '/usr/local/lsws/lsphp5');

$_tipsdb['instances'] = new DAttrHelp("インスタンス", 'サーバーが作成する外部アプリケーションの最大インスタンスを指定します。 &quot;自動スタート&quot;が有効な場合は必須です。 ほとんどのFastCGI/LSAPIアプリケーションは、プロセスインスタンスごとに1つの要求しか処理できません。これらのタイプのアプリケーションの場合、インスタンスは&quot;最大接続数&quot;の値に一致するように設定する必要があります。 一部のFastCGI / LSAPIアプリケーションでは、複数の子プロセスを生成して複数の要求を同時に処理できます。 これらのタイプのアプリケーションでは、インスタンスを &quot;1&quot;に設定し、環境変数を使用してアプリケーションが生成できる子プロセスの数を制御する必要があります。', '', '整数', '');

$_tipsdb['internalmodule'] = new DAttrHelp("内部", 'モジュールが外部の.soライブラリではなく、静的にリンクされている内部モジュールであるかどうかを指定します。', '', 'ラジオボックスから選択', '');

$_tipsdb['ip2locDBCache'] = new DAttrHelp("DBキャッシュタイプ", '使用するキャッシュ方法です。デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Memory</span>です。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['ip2locDBFile'] = new DAttrHelp("IP2Location DBファイルパス", '有効なデータベースファイルの場所です。', '', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス。', '');

$_tipsdb['javaServletEngine'] = new DAttrHelp("サーブレットエンジン", 'このWebアプリケーションを提供するサーブレットエンジンの名前を指定します。 サーブレットエンジンは、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['javaWebAppContext'] = new DAttrHelp("Java Webアプリコンテキスト", 'Javaアプリケーションを実行している多くの人々は、サーブレットエンジンを使用して静的コンテンツを提供しています。 しかし、サーブレットエンジンは、これらのプロセスではLiteSpeed Web Serverほど効率的ではありません。 全体のパフォーマンスを向上させるために、LiteSpeed Web Serverをゲートウェイサーバーとして構成することができます。ゲートウェイサーバーは静的コンテンツを処理し、動的Javaページ要求をサーブレットエンジンに転送します。<br/><br/>LiteSpeed Web Serverでは、Javaアプリケーションを実行するために特定のコンテキストを定義する必要があります。 Java Webアプリケーションコンテキストは、Java Webアプリケーションの構成ファイル（WEB-INF/web.xml）に基づいて、必要なすべてのコンテキストを自動的に作成します。<br/><br/>Java Webアプリケーションコンテキストを設定する際に留意すべき点がいくつかあります：<br/><ul> <li>Java Webアプリコンテキストを設定する前に、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>サーブレットエンジン</span>外部アプリケーションを&quot;外部アプリケーション&quot;に設定する必要があります</li> <li>.jspファイルの&quot;スクリプトハンドラ&quot;も同様に定義する必要があります。</li> <li>Webアプリケーションが.warファイルにパックされている場合は、.warファイルを展開する必要があります。サーバーは圧縮されたアーカイブファイルにアクセスできません。</li><li>同じリソースの場合、LiteSpeed Web Serverを介してアクセスするか、サーブレットエンジンの組み込みHTTPサーバーを介してアクセスするかにかかわらず、同じURLを使用する必要があります。<br> <br> 例えば、<br> Tomcat 4.1は/ opt / tomcatにインストールされます。<br> &quot;examples&quot; Webアプリケーションのファイルは、/opt/tomcat/webapps/examples/にあります。<br> TomcatのビルトインHTTPサーバーを通じて、 &quot;examples&quot; Webアプリケーションは &quot;/ examples / ***&quot;のようなURIでアクセスされます。<br> したがって、対応するJava Webアプリコンテキストを設定する必要があります： <br> URI = <span class=&quot;lst-inline-token lst-inline-token--value&quot;>/examples/</span>, Location = <span class=&quot;lst-inline-token lst-inline-token--value&quot;>/opt/tomcat/webapps/examples/</span>.</li> </ul>', '', '', '');

$_tipsdb['javaWebApp_location'] = new DAttrHelp("ロケーション", 'このWebアプリケーションのファイルを含むディレクトリを指定します。 これは &quot;WEB-INF/web.xml&quot;を含むディレクトリです。', '', 'パス', '');

$_tipsdb['jsonReports'] = new DAttrHelp("JSONレポートを出力", '.json拡張子を持つ追加のJSON形式レポートファイルを/tmp/lshttpdディレクトリへ出力します。<br/><br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '[情報] これは、ほとんどのプログラミング言語に組み込まれている標準JSON処理ツールを使用して、 LiteSpeedステータスとリアルタイムレポートを自分のアプリケーションへ統合したいアプリケーション開発者に便利です。', 'ラジオボックスから選択', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定すると、通常の.status、.rtreport、.rtreport.2などのレポートファイルに加えて、 .status.json、.rtreport.json、.rtreport.2.jsonなどのレポートファイルが作成されます。');

$_tipsdb['keepAliveTimeout'] = new DAttrHelp("キープアライブタイムアウト（秒）", 'キープアライブ接続からの要求間の最大アイドル時間を指定します。 この期間中に新しい要求が受信されない場合、接続は閉じられます。 この設定は、HTTP/1.1接続にのみ適用されます。 HTTP/2接続は、設計によって長いキープアライブタイムアウトを持ち、この設定の影響を受けません。', '[セキュリティ & パフォーマンス]ロードする必要がある単一のページで参照されるアセットが多くある場合、クライアントからの後続のリクエストを待機するのに十分な時間だけこの値を設定することをお勧めします。 キープアライブ接続で次のページが配信されることを期待して、これを長く設定しないでください。 多くのアイドル状態のキープアライブ接続を維持することはサーバーリソースの浪費であり、（D）DoS攻撃によって活用される可能性があります。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>2-5</span>秒はほとんどのアプリケーションにとって妥当な範囲です。 LiteSpeedは非キープアライブ環境で非常に効率的です。', '整数', '');

$_tipsdb['keyFile'] = new DAttrHelp("秘密鍵ファイル", 'SSL秘密鍵ファイルのファイル名。キーファイルは暗号化しないでください。', '[セキュリティ]秘密鍵ファイルは、サーバーが実行されるユーザーへの読み取り専用アクセスを可能にするセキュリティ保護されたディレクトリに配置する必要があります。', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['lbContext'] = new DAttrHelp("ロードバランサコンテキスト", '他の外部アプリケーションと同様に、ロードバランサワーカーアプリケーションは直接使用できません。 それらは、コンテキストを介してURLにマップされなければなりません。 ロードバランサコンテキストは、ロードバランサワーカーによって負荷分散されるURIを関連付けます。', '', '', '');

$_tipsdb['lbapp'] = new DAttrHelp("ロードバランサー", 'このコンテキストに関連付けるロードバランサの名前を指定します。 このロードバランサはバーチャルアプリケーションであり、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります', '', 'ドロップダウンリストから選択', '');

$_tipsdb['listenerBinding'] = new DAttrHelp("バインディング", 'リスナーが割り当てられているlshttpd子プロセスを指定します。 リスナーをプロセスに手動で関連付けることによって、異なるリスナーへのリクエストを処理するために、異なる子プロセスを使用できます。 デフォルトでは、すべての子プロセスにリスナーが割り当てられます。', '', 'チェックボックスから選択', '');

$_tipsdb['listenerIP'] = new DAttrHelp("IPアドレス", 'このリスナーのIPを指定します。利用可能なすべてのIPアドレスが一覧表示されます。IPv6アドレスは&quot;[ ]&quot;で囲まれます。<br/><br/>すべてのIPv4 IPアドレスで待ち受けるには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ANY</span>を選択します。すべてのIPv4およびIPv6 IPアドレスで待ち受けるには、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>[ANY]</span>を選択します。<br/><br/>IPv4とIPv6の両方のクライアントにサービスを提供するには、通常のIPv4アドレスではなくIPv4マップIPv6アドレスを使用してください。 IPv4マップIPv6アドレスは[::FFFF:x.x.x.x]の形式で記述します。', '[セキュリティ] マシンに異なるサブネットワーク上の複数のIPがある場合、特定のIPを選択して、 対応するサブネットワークからのトラフィックのみを許可できます。', 'ドロップダウンリストから選択', '');

$_tipsdb['listenerModules'] = new DAttrHelp("リスナーモジュール", 'リスナーモジュール設定データは、デフォルトでサーバーモジュール設定から継承されます。 リスナモジュールは、TCP / IPレイヤ4フックに限定されています。', '', '', '');

$_tipsdb['listenerName'] = new DAttrHelp("リスナー名", 'このリスナーの一意の名前。', '', 'テキスト', '');

$_tipsdb['listenerPort'] = new DAttrHelp("ポート", 'リスナーのTCPポートを指定します。 スーパーユーザー（&quot;root&quot;）のみが<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1024</span>より小さいポートを使用できます。 ポート<span class=&quot;lst-inline-token lst-inline-token--value&quot;>80</span>はデフォルトのHTTPポートです。 ポート<span class=&quot;lst-inline-token lst-inline-token--value&quot;>443</span>はデフォルトのHTTPSポートです。', '', '整数', '');

$_tipsdb['listenerSecure'] = new DAttrHelp("セキュア", 'これがセキュア（SSL）リスナーかどうかを指定します。 セキュアリスナーの場合は、追加のSSL設定を適切に設定する必要があります。', '', 'ラジオボックスから選択', '');

$_tipsdb['lmap'] = new DAttrHelp("バーチャルホストマッピング", '特定のリスナーからのバーチャルホストへの現在確立されているマッピングを表示します。 バーチャルホスト名は括弧内に表示され、このリスナーに一致するドメイン名が続きます。', 'バーチャルホストが正常にロードされていない場合（バーチャルホストの致命的なエラー）、そのバーチャルホストへのマッピングは表示されません。', '', '');

$_tipsdb['lname'] = new DAttrHelp("名前 - リスナー", 'このリスナーを識別する固有の名前。 これは、リスナーを設定するときに指定した&quot;リスナー名&quot;です。', '', '', '');

$_tipsdb['location'] = new DAttrHelp("場所", 'ファイルシステム内で、このコンテキストに対応する場所を指定します。<br/><br/>デフォルト値：$DOC_ROOT + &quot;URI&quot;', '', '絶対パス、または$SERVER_ROOT、$VH_ROOT、$DOC_ROOTからの相対パスを指定できます。 $DOC_ROOTはデフォルトの相対パスであり、省略できます。<br/><br/>&quot;URI&quot;が正規表現の場合、一致した部分文字列を使用して&quot;Root&quot;文字列を構成できます。一致した部分文字列は&quot;$1&quot;から&quot;$9&quot;の値で参照できます。&quot;$0&quot;と&quot;&&quot;を使用して、一致した文字列全体を参照することもできます。さらに、&quot;?&quot;の後にクエリ文字列を追加して、クエリ文字列を設定できます。注意してください。クエリ文字列内の&quot;&&quot;は&quot;\&&quot;としてエスケープする必要があります。', '&quot;場所&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/john/web_examples</span>に設定した<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/examples/</span>のような通常のURIは、リクエスト&quot;/examples/foo/bar.html&quot;をファイル&quot;/home/john/web_examples/foo/bar.html&quot;にマップします。<br/>Apacheのmod_userdirをシミュレートするには、URIを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>exp: ^/~([A-Za-z0-9]+)(.*)</span>に設定し、&quot;場所&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/$1/public_html$2</span>に設定します。この設定では、URI/~john/foo/bar.htmlのリクエストはファイル/home/john/public_html/foo/bar.htmlにマップされます。');

$_tipsdb['logUseServer'] = new DAttrHelp("サーバーのログを使用する", '独自のログファイルを作成するのではなく、このバーチャルホストからのログメッセージをサーバーログファイルに入れるかどうかを指定します。', '', 'ラジオボックスから選択', '');

$_tipsdb['log_compressArchive'] = new DAttrHelp("アーカイブを圧縮する", 'ディスク領域を節約するためにローテーションしたログファイルを圧縮するかどうかを指定します。', 'ログファイルは圧縮率が高いため、古いログのディスク使用量を減らす目的で推奨されます。', 'ラジオボックスから選択', '');

$_tipsdb['log_debugLevel'] = new DAttrHelp("デバッグレベル", 'デバッグログのレベルを指定します。 この機能を使用するには、&quot;ログレベル&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>に設定する必要があります。 &quot;ログレベル&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>に設定されていても、「デバッグレベル」が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>に設定されていると、デバッグログは無効になります。 &quot;デバッグログを切り替える&quot;を使用すると、稼働中のサーバーで再起動せずにデバッグレベルを制御できます。', '[パフォーマンス] 重要！詳細なデバッグログが必要ない場合は、必ず<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>に設定してください。 有効なデバッグログはサービス性能を著しく低下させ、非常に短時間でディスク容量を使い切る可能性があります。 デバッグログには、各リクエストとレスポンスの詳細情報が含まれます。<br/>ログレベルを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>に設定し、デバッグレベルを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>に設定することをお勧めします。 この設定ではデバッグログでディスクを埋めることなく、&quot;デバッグログを切り替える&quot;アクションでデバッグ出力を制御できます。 このアクションはデバッグログを即座にオン/オフできるため、多忙な本番サーバーのデバッグに役立ちます。', 'ドロップダウンリストから選択', '');

$_tipsdb['log_enableStderrLog'] = new DAttrHelp("stderrログを有効にする", 'サーバーが開始したプロセスからstderr出力を受信したときにログへ書き込むかどうかを指定します。<br/><br/>有効にすると、stderrメッセージはサーバーログと同じディレクトリに、固定名&quot;stderr.log&quot;で記録されます。 無効にすると、すべてのstderr出力は破棄されます。<br/><br/>stderr（ファイルハンドル2）へ直接書き込まないPHPのerror_log()などの関数は、この設定の影響を受けません。 これらはPHP iniディレクティブ&#039;error_log&#039;で設定されたファイル、またはそのディレクティブが未設定の場合はサーバーの&quot;error.log&quot;ファイルへ&#039;[STDERR]&#039;タグ付きで書き込みます。', 'PHP、Ruby、Java、Python、Perlなど、設定済み外部アプリケーションをデバッグする必要がある場合は有効にします。', 'ラジオボックスから選択', '');

$_tipsdb['log_fileName'] = new DAttrHelp("ファイル名", 'ログファイルのパスを指定します。', '[パフォーマンス] ログファイルは別のディスクに配置してください。', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス。', '');

$_tipsdb['log_keepDays'] = new DAttrHelp("保持日数", 'アクセスログファイルをディスク上に保持する日数を指定します。 指定した日数より古いローテーション済みログファイルのみが削除されます。 現在のログファイルは、含まれるデータの日数に関係なく変更されません。 古くなった非常に古いログファイルを自動削除したくない場合は、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定します。', '', '整数', '');

$_tipsdb['log_logLevel'] = new DAttrHelp("ログレベル", 'ログファイルに含めるログのレベルを指定します。 使用可能なレベルは、高い順に<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ERROR</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>WARNING</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NOTICE</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>INFO</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>です。 現在の設定以上のレベルのメッセージのみが記録されます。', '[パフォーマンス] &quot;デバッグレベル&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>以外のレベルに設定されていない限り、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>ログレベルを使用してもパフォーマンスに影響はありません。 ログレベルを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>に設定し、デバッグレベルを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>に設定することをお勧めします。 この設定ではデバッグログでディスクを埋めることなく、&quot;デバッグログを切り替える&quot;アクションでデバッグ出力を制御できます。 このアクションはデバッグログを即座にオン/オフできるため、多忙な本番サーバーのデバッグに役立ちます。', 'ドロップダウンリストから選択', '');

$_tipsdb['log_rollingSize'] = new DAttrHelp("ローテーションサイズ（バイト）", '現在のログファイルをロールオーバーする必要があるとき、つまりログローテーションを指定します。 ファイルサイズがロールオーバー制限を超えると、アクティブなログファイルは同じディレクトリにlog_name.mm_dd_yyyy（.sequence）という名前に変更され、新しいアクティブなログファイルが作成されます。 ローテーション済みログファイルが実際に作成されると、そのサイズはこの制限より少し大きくなることがあります。ログローテーションを無効にするには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定します。', '数値に&quot;K&quot;、&quot;M&quot;、&quot;G&quot;を付けると、それぞれキロバイト、メガバイト、ギガバイトを表します。', '整数', '');

$_tipsdb['loggerAddress'] = new DAttrHelp("リモートロガーアドレス(オプション)", 'このパイプログロガーで使用する任意のソケットアドレスを指定します。 ロガーにネットワークソケットまたはUnixドメインソケットで接続する場合に設定します。 設定したコマンドパスから起動するローカルパイプログロガーでは空のままにできます。', '', 'IPv4アドレス:ポート、ホスト名:ポート、[IPv6アドレス]:ポート、UDS://path、またはunix:path', '127.0.0.1:1514<br/>logger.example.com:1514<br/>UDS://tmp/lshttpd/logger.sock');

$_tipsdb['loginHistoryRetention'] = new DAttrHelp("ログイン履歴保持期間(日)", '古いエントリが削除されるまでログイン履歴レコードを保持する日数を指定します。未設定時のデフォルト値:90日。', '監査とトラブルシューティングに十分な履歴を保持しつつ、実際に必要な量を超えるデータ保持は避けてください。', '整数', '');

$_tipsdb['loginThrottle'] = new DAttrHelp("ログインスロットル", 'WebAdminコンソールのログインスロットルと、関連するログインおよび監査レコードの保持を設定します。', '', '', '');

$_tipsdb['lsapiContext'] = new DAttrHelp("LiteSpeed SAPIコンテキスト", '外部アプリケーションは直接使用することはできません。 スクリプトハンドラとして設定するか、コンテキストを介してURLにマップする必要があります。 LiteSpeed SAPIコンテキストは、URIをLSAPI（LiteSpeed Server Application Programming Interface）アプリケーションに関連付けます。 現在PHP、Ruby、PythonにはLSAPIモジュールがあります。 LiteSpeed Webサーバー用に特別に開発されたLSAPIは、LiteSpeed Webサーバーと通信するための最も効率的な方法です。', '', '', '');

$_tipsdb['lsapiapp'] = new DAttrHelp("LiteSpeed SAPIアプリ", 'このコンテキストに接続するLiteSpeed SAPIアプリケーションの名前を指定します。 このアプリケーションは、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['lsrecaptcha'] = new DAttrHelp("CAPTCHA保護", 'CAPTCHA保護は、高いサーバー負荷を緩和する手段として提供されるサービスです。CAPTCHA保護は、以下のいずれかの状況に達すると有効化されます。有効化されると、信頼されていない（設定に基づく）クライアントからのすべてのリクエストはCAPTCHA検証ページにリダイレクトされます。検証後、クライアントは目的のページにリダイレクトされます。<br/><br/>次の状況でCAPTCHA保護が有効化されます：<br/>1. サーバーまたはvhostの同時リクエスト数が、設定された接続数上限を超えた場合。<br/>2. Anti-DDoSが有効で、クライアントが疑わしい方法でURLにアクセスしている場合。トリガーされると、拒否される前にクライアントはまずCAPTCHAにリダイレクトされます。<br/>3. CAPTCHAをRewriteRulesで有効化するための新しいrewrite rule環境が提供されています。&#039;verifycaptcha&#039;を設定すると、クライアントをCAPTCHAにリダイレクトできます。特殊な値&#039;: deny&#039;を設定すると、失敗回数が多すぎるクライアントを拒否できます。たとえば、[E=verifycaptcha]は検証されるまで常にCAPTCHAにリダイレクトします。[E=verifycaptcha: deny]は最大試行回数に達するまでCAPTCHAにリダイレクトし、その後クライアントを拒否します。', '', '', '');

$_tipsdb['lstatus'] = new DAttrHelp("ステータス - リスナー", 'このリスナーの現在のステータス。 ステータスは<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running</span>か<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Error</span>のいずれかです。', 'リスナーが<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Error</span>状態にある場合は、サーバーログを表示して理由を調べることができます。', '', '');

$_tipsdb['mappedListeners'] = new DAttrHelp("マッピングされたリスナー", 'このテンプレートがマップするすべてのリスナー名を指定します。 このテンプレートのメンバーバーチャルホスト用のリスナーからバーチャルホストへのマッピングが、このフィールドで指定されたリスナーに追加されます。 このマッピングは、各メンバーバーチャルホストの個別設定にあるドメイン名とエイリアスに基づいて、リスナーをバーチャルホストにマップします。', '', 'カンマ区切りリスト', '');

$_tipsdb['maxCGIInstances'] = new DAttrHelp("最大CGIインスタンス", 'サーバーが開始できる同時CGIプロセスの最大数を指定します。 CGIスクリプトに対する各要求に対して、サーバーはスタンドアロンCGIプロセスを開始する必要があります。 Unixシステムでは、並行プロセスの数が制限されています。 過度の並行処理は、システム全体のパフォーマンスを低下させ、DoS攻撃を実行する1つの方法です。 LiteSpeedサーバーはCGIスクリプトへの要求をパイプライン処理し、同時のCGIプロセスを制限して最適なパフォーマンスと信頼性を確保します。 上限は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>2000</span>です。', '[セキュリティ & パフォーマンス]上限が高いと必ずしもパフォーマンスが向上するとは限りません。 ほとんどの場合、下限値を指定するとパフォーマンスとセキュリティが向上します。 上限は、CGI処理中にI / O待ち時間が過大になる場合にのみ役立ちます。', '整数', '');

$_tipsdb['maxCachedFileSize'] = new DAttrHelp("最大キャッシュサイズの小さいファイルサイズ（バイト）", '事前に割り当てられたメモリバッファにキャッシュされる最大の静的ファイルを指定します。 静的ファイルは、メモリバッファキャッシュ、メモリマップキャッシュ、プレーンリード/ライト、およびsendfile（）の4つの方法で提供できます。 サイズがこの設定より小さいファイルはメモリバッファキャッシュから提供されます。 サイズがこの設定より大きく、&quot;最大MMAPファイルサイズ（バイト）&quot;より小さいファイルは、メモリマップドキャッシュから処理されます。 &quot;最大MMAPファイルサイズ（バイト）&quot;より大きいサイズのファイルは、プレーン・リード/ライトまたはsendfile（）を介して処理されます。 メモリバッファキャッシュから4Kより小さい静的ファイルを提供することが最適です。', '', '整数', '');

$_tipsdb['maxConnections'] = new DAttrHelp("最大接続数", 'サーバーが受け入れることができる同時接続の最大数を指定します。 これには、プレーンTCP接続とSSL接続の両方が含まれます。 最大同時接続制限に達すると、サーバーはアクティブな要求が完了するとキープアライブ接続を閉じます。', 'サーバーが &quot;root&quot;ユーザーによって起動されると、サーバーはプロセスごとのファイル記述子の制限を自動的に調整しようとしますが、失敗した場合は手動でこの制限を増やす必要があります。', '整数', '');

$_tipsdb['maxConns'] = new DAttrHelp("最大接続数", 'サーバーと外部アプリケーションの間で確立できる同時接続の最大数を指定します。 この設定は、外部アプリケーションによって同時に処理できる要求の数を制御しますが、実際の制限は外部アプリケーション自体によっても異なります。 この値を高く設定すると、外部アプリケーションの速度が不十分であるか、多数の同時要求に対応できない場合に役立ちません。', '[パフォーマンス]高い値を設定しても、高いパフォーマンスに直接変換されるわけではありません。 制限を外部アプリケーションに負荷をかけない値に設定すると、最高のパフォーマンス/スループットが得られます。', '整数', '');

$_tipsdb['maxDynRespHeaderSize'] = new DAttrHelp("最大動的レスポンスヘッダーサイズ(バイト)", '動的に生成されるレスポンスの最大ヘッダーサイズを指定します。ハードリミットは<span class=&quot;lst-inline-token lst-inline-token--value&quot;>131072</span>バイト、つまり128Kです。<br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32768</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32K</span>', '[パフォーマンス] 外部アプリケーションによって動的に生成される不正なレスポンスを認識しやすくするため、適度に低く設定してください。', '整数', '');

$_tipsdb['maxDynRespSize'] = new DAttrHelp("最大動的レスポンスボディサイズ(バイト)", '動的に生成されるレスポンスの最大ボディサイズを指定します。', '[パフォーマンス] 不正なレスポンスを識別しやすくするため、制限を適度に低く設定してください。不正なスクリプトに無限ループが含まれ、無限大のレスポンスにつながることは珍しくありません。', '整数', '');

$_tipsdb['maxKeepAliveReq'] = new DAttrHelp("最大キープアライブ要求", 'キープアライブ（永続的）セッションを介して処理できる要求の最大数を指定します。 この制限に達すると接続は終了します。 バーチャルホストごとにこの制限を設定することもできます。', '[パフォーマンス]適度に高い値に設定します。 &quot;1&quot;または &quot;0&quot;の値はキープアライブを無効にします。', '整数', '');

$_tipsdb['maxMMapFileSize'] = new DAttrHelp("最大MMAPファイルサイズ（バイト）", 'メモリマップされる最大の静的ファイル（MMAP）を指定します。   静的ファイルは、メモリバッファキャッシュ、メモリマップキャッシュ、プレーンリード/ライト、およびsendfile（）の4つの方法で提供できます。 サイズが&quot;最大キャッシュサイズの小さいファイルサイズ（バイト）&quot;より小さいファイルはメモリバッファキャッシュから提供されます。 サイズが&quot;最大キャッシュサイズの小さいファイルサイズ（バイト）&quot;よりも大きいが最大MAPファイルサイズより小さいファイルは、メモリマップドキャッシュから提供されます。 最大MMAPファイルサイズよりも大きいファイルは、プレーンな読み取り/書き込みまたはsendfile（）を介して提供されます。 サーバは32ビットのアドレス空間（2GB）を持っているので、非常に大きなファイルをメモリに格納することは推奨されません。', '', '整数', '');

$_tipsdb['maxMindDBEnv'] = new DAttrHelp("環境変数", 'データベース検索結果を環境変数に割り当てます。', '', 'Variable_Name mapped_DB_data<br/><br/>1行に1エントリです。データへのパスにはマップキーまたは0ベースの配列インデックスを使用でき、どちらも/で区切ります。', 'COUNTRY_CODE COUNTRY_DB/country/iso_code<br/>REGION_CODE  CITY_DB/subdivisions/0/iso_code');

$_tipsdb['maxReqBodySize'] = new DAttrHelp("最大リクエストボディサイズ（バイト）", 'HTTPリクエスト本文の最大サイズを指定します。 32ビットOSの場合、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>2GB</span>はハード制限です。 64ビットOSの場合、実質的に無制限です。', '[セキュリティ] DoS攻撃を防ぐには、この制限を実際に必要なものだけに制限してください。 スワッピング空間には、この制限に対応するための十分な空き領域が必要です。', '整数', '');

$_tipsdb['maxReqHeaderSize'] = new DAttrHelp("最大リクエストヘッダーサイズ(バイト)", 'リクエストURLを含むHTTPリクエストヘッダーの最大サイズを指定します。ハードリミットは<span class=&quot;lst-inline-token lst-inline-token--value&quot;>131072</span>バイト、つまり128Kです。<br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32768</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32K</span>', '[セキュリティ][パフォーマンス] メモリ使用量を減らし、不正なリクエストやDoS攻撃を識別しやすくするため、適度に低く設定してください。<br/>通常の状況では、ほとんどのWebサイトで4-8Kで十分です。', '整数', '');

$_tipsdb['maxReqURLLen'] = new DAttrHelp("最大リクエストURL長(バイト)", 'リクエストURLの最大サイズを指定します。URLは、クエリ文字列を含む、サーバーリソースへのアクセスに使用される完全なテキストアドレスです。ハードリミットは<span class=&quot;lst-inline-token lst-inline-token--value&quot;>65530</span>バイトです。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>64K</span>（6バイト超過）など、これより大きい値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>65530</span>が使用されたものとして扱われます。<br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>8192</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>8K</span>。', '[セキュリティ][パフォーマンス] メモリ使用量を減らし、不正なリクエストやDoS攻撃を識別しやすくするため、適度に低く設定してください。<br/>POSTの代わりにHTTP GETメソッドで大きなクエリ文字列を使用しない限り、ほとんどのWebサイトでは2-3Kで十分です。', '整数', '');

$_tipsdb['maxSSLConnections'] = new DAttrHelp("最大SSL接続数", 'サーバーが受け入れる同時SSL接続の最大数を指定します。 同時SSL接続と非SSL接続の合計が&quot;最大接続数&quot;で指定された制限を超えることはできないため、許可される同時SSL接続の実際の数はこの制限より小さくなければなりません。', '', '整数', '');

$_tipsdb['memHardLimit'] = new DAttrHelp("メモリハードリミット", 'ソフトリミットをユーザープロセス内のハードリミットまで上げることができることを除けば、&quot;メモリソフトリミット（バイト）&quot;と同じくらい同じです。 ハード・リミットは、サーバー・レベルまたは個々の外部アプリケーション・レベルで設定できます。 サーバーレベルの制限は、個々のアプリケーションレベルで設定されていない場合に使用されます。<br/><br/>値が両方のレベルにないか、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定されている場合、オペレーティングシステムのデフォルトが使用されます。', '[注意]この制限を過度に調整しないでください。アプリケーションがより多くのメモリを必要とする場合、503エラーが発生する可能性があります。', '整数', '');

$_tipsdb['memSoftLimit'] = new DAttrHelp("メモリソフトリミット（バイト）", '外部アプリケーション・プロセスまたはサーバーによって開始された外部アプリケーションのメモリー消費制限をバイト単位で指定します。<br/><br/>この制限の主な目的は、ソフトウェアのバグや意図的な攻撃のために過度のメモリ使用を防止し、通常の使用に制限を設けないことです。 十分なヘッドスペースを確保してください。そうしないと、アプリケーションが失敗し、503エラーが返される可能性があります。 サーバーレベルまたは個々の外部アプリケーションレベルで設定できます。 サーバーレベルの制限は、個々のアプリケーションレベルで設定されていない場合に使用されます。<br/><br/>オペレーティングシステムのデフォルト設定は、値が両方のレベルにないか、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定されている場合に使用されます。', '[注意]この制限を過度に調整しないでください。 アプリケーションでより多くのメモリが必要な場合は、503のエラーが発生する可能性があります。', '整数', '');

$_tipsdb['memberVHRoot'] = new DAttrHelp("メンバーバーチャルホストルート", 'このバーチャルホストのルートディレクトリを指定します。 ブランクのままにすると、このテンプレートのデフォルトのバーチャルホストルートが使用されます。<br/><br/>注意: これはドキュメントルートでは<b>ありません</b>。 バーチャルホストに関連するすべてのファイル（バーチャルホスト設定、ログファイル、htmlファイル、CGIスクリプトなど）をこのディレクトリの下に配置することをお勧めします。 バーチャルホストルートは$VH_ROOTとして参照できます。', '', 'パス', '');

$_tipsdb['mime'] = new DAttrHelp("MIME設定", 'このサーバーのMIME設定を含むファイルを指定します。 chrootモードで絶対パスが指定されている場合、実際のルートと常に相対的です。 詳細なMIMEエントリを表示/編集するには、ファイル名をクリックします。', 'ファイル名をクリックしてMIME設定を編集します。', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['mimesuffix'] = new DAttrHelp("サフィックス", '同じMIMEタイプの複数のサフィックスをカンマで区切って指定することができます。', '', '', '');

$_tipsdb['mimetype'] = new DAttrHelp("MIMEタイプ", 'MIMEタイプは、タイプとサブタイプの形式で &quot;タイプ/サブタイプ&quot;で構成されます。', '', '', '');

$_tipsdb['minGID'] = new DAttrHelp("最小GID", '外部アプリケーションの最小グループIDを指定します。 ここで指定した値よりも小さいグループIDを持つ外部の実行は拒否されます。 LiteSpeed Webサーバーが &quot;root&quot;ユーザーによって起動された場合、Apacheで見つかった &quot;suEXEC&quot;モードで外部アプリケーションを実行できます（Webサーバー以外のユーザー/グループIDに変更する）。', '[セキュリティ]システムユーザーが使用するすべてのグループを除外するのに十分な高さに設定します。', '整数', '');

$_tipsdb['minUID'] = new DAttrHelp("最小UID", '外部アプリケーションの最小ユーザーIDを指定します。 ここで指定した値よりも小さいユーザーIDを持つ外部スクリプトの実行は拒否されます。 LiteSpeed Webサーバーが「root」ユーザーによって起動された場合、Apacheなどの「suEXEC」モードで外部アプリケーションを実行できます（Webサーバー以外のユーザー/グループIDに変更する）。', '[セキュリティ]すべてのシステム/特権ユーザーを除外するのに十分な高さに設定します。', '整数', '');

$_tipsdb['modParams'] = new DAttrHelp("モジュールのパラメータ", 'モジュールのパラメータを設定します。 モジュールパラメータは、モジュール開発者によって定義されます。<br/><br/>デフォルト値をグローバルに割り当てるようにサーバー構成内の値を設定します。 ユーザーは、この設定をリスナー、バーチャルホストまたはコンテキスト・レベルでオーバーライドできます。 「未設定」ラジオボタンが選択されている場合、それは上位レベルから継承されます。', '', 'モジュールインタフェースで指定されます。', '');

$_tipsdb['moduleContext'] = new DAttrHelp("モジュールハンドラコンテキスト", 'モジュールハンドラコンテキストは、URIを登録済みモジュールに関連付けます。 モジュールはサーバーモジュール設定タブで登録する必要があります。', '', '', '');

$_tipsdb['moduleEnabled'] = new DAttrHelp("フックを有効にする", 'モジュールフックをグローバルに有効または無効にします。<br/>「未設定」ラジオボタンが選択され、モジュールにフック機能が含まれている場合、デフォルトが有効になります。 ユーザーは、各レベルでこのグローバル設定を上書きできます。', '', 'ラジオボックスから選択', '');

$_tipsdb['moduleEnabled_lst'] = new DAttrHelp("フックを有効にする", 'リスナーレベルでモジュールフックを有効または無効にします。 モジュールにTCP/IPレベルのフック（L4_BEGSESSION、L4_ENDSESSION、L4_RECVING、L4_SENDING）がある場合のみ、この設定が有効になります。<br/><br/>「未設定」ラジオボタンが選択されている場合、デフォルトはサーバー設定から継承されます。 ユーザーはデフォルト設定を上書きするためにここで設定するだけです。', '', 'ラジオボックスから選択', '');

$_tipsdb['moduleEnabled_vh'] = new DAttrHelp("フックを有効にする", 'バーチャルホストまたはコンテキスト・レベルでモジュール・フックを使用可能または使用不可にします。 モジュールにHTTPレベルのフックがある場合のみ、この設定が有効になります。<br/><br/>「未設定」ラジオボタンを選択すると、バーチャルホストレベルのデフォルト設定がサーバー設定から継承されます。 コンテキストレベルの設定は、バーチャルホストレベルから継承されます。 ユーザーはデフォルト設定を上書きするためにここで設定するだけです。', '', 'ラジオボックスから選択', '');

$_tipsdb['moduleNameSel'] = new DAttrHelp("モジュール", 'モジュールの名前。 モジュールはサーバーモジュールの設定タブに登録する必要があります。 登録されると、モジュール名がリスナーおよび仮想ホスト構成のドロップダウンボックスで使用可能になります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['modulename'] = new DAttrHelp("モジュール", 'モジュールの名前。 モジュール名はモジュールファイル名と同じになります。 モジュール・ファイルは、サーバー・アプリケーションによってロードされるために$SERVER_ROOT/modules/modulename.soの下になければなりません。 サーバーは起動時に登録されたモジュールをロードします。 このためには、新しいモジュールが登録された後でサーバーを再起動する必要があります。', '', '.soのライブラリ名。', '');

$_tipsdb['namespace'] = new DAttrHelp("Namespaceコンテナ", 'CGIプロセス（PHPプログラムを含む）をNamespaceコンテナサンドボックス内で起動する場合は、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Enabled</span>に設定します。&quot;Bubblewrapコンテナ&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>に設定されている場合にのみ使用されます。<br/><br/>サーバーレベルで<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>以外に設定されている場合、この設定値はバーチャルホストレベルで上書きできます。<br/><br/>デフォルト値：<br/><b>サーバーレベル：</b><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span><br/><b>バーチャルホストレベル：</b>サーバーレベル設定を継承', '', 'ドロップダウンリストから選択', '');

$_tipsdb['namespaceConf'] = new DAttrHelp("Namespaceテンプレートファイル", 'マウントするディレクトリのリストと、それらをマウントする方法を含む既存の設定ファイルへのパスです。&quot;Namespaceコンテナ&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Enabled</span>に設定され、この値が未設定の場合、次の安全なデフォルト設定が使用されます。<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;> $HOMEDIR/.lsns/tmp /tmp,tmp<br/>/usr,ro-bind<br/>/lib,ro-bind<br/>/lib64,ro-bind-try<br/>/bin,ro-bind<br/>/sbin,ro-bind<br/>/var,dir<br/>/var/www,ro-bind-try<br/>/proc,proc<br/>../tmp var/tmp,symlink<br/>/dev,dev<br/>/etc/localtime,ro-bind-try<br/>/etc/ld.so.cache,ro-bind-try<br/>/etc/resolv.conf,ro-bind-try<br/>/etc/ssl,ro-bind-try<br/>/etc/pki,ro-bind-try<br/>/etc/man_db.conf,ro-bind-try<br/>/usr/local/bin/msmtp /etc/alternatives/mta,ro-bind-try<br/>/usr/local/bin/msmtp /usr/sbin/exim,ro-bind-try<br/>$HOMEDIR,bind-try<br/>/var/lib/mysql/mysql.sock,bind-try<br/>/home/mysql/mysql.sock,bind-try<br/>/tmp/mysql.sock,bind-try<br/>/run/mysqld/mysqld.sock,bind-try<br/>/var/run/mysqld.sock,bind-try<br/>/run/user/$UID,bind-try<br/>$PASSWD<br/>$GROUP<br/>/etc/exim.jail/$USER.conf $HOMEDIR/.msmtprc,copy-try<br/>/etc/php.ini,ro-bind-try<br/>/etc/php-fpm.conf,ro-bind-try<br/>/etc/php-fpm.d,ro-bind-try<br/>/var/run,ro-bind-try<br/>/var/lib,ro-bind-try<br/>/etc/imunify360/user_config/,ro-bind-try<br/>/etc/sysconfig/imunify360,ro-bind-try<br/>/opt/plesk/php,ro-bind-try<br/>/opt/alt,bind-try<br/>/opt/cpanel,bind-try<br/>/opt/psa,bind-try<br/>/var/lib/php/sessions,bind-try </span>', '', '絶対パス又は$SERVER_ROOTからの相対パス。', '');

$_tipsdb['namespaceConfVhAdd'] = new DAttrHelp("追加Namespaceテンプレートファイル", 'マウントするディレクトリのリストと、それらをマウントするために使用する方式を含む既存設定ファイルへのパスです。サーバーレベルで&quot;Namespaceテンプレートファイル&quot;も設定されている場合は、両方のファイルが使用されます。', '', '絶対パスか$SERVER_ROOTからの相対パス又は$VH_ROOTからの相対パス。', '');

$_tipsdb['nodeBin'] = new DAttrHelp("Node.jsパス", 'Node.js実行ファイルへのパスです。', '', '絶対パス', '');

$_tipsdb['nodeDefaults'] = new DAttrHelp("Node.jsアプリデフォルト設定", 'Node.jsアプリケーションのデフォルト設定です。これらの設定はコンテキストレベルで上書きできます。', '', '', '');

$_tipsdb['note'] = new DAttrHelp("メモ", '自分用のメモを追加します。', '', '', '');

$_tipsdb['ocspCACerts'] = new DAttrHelp("OCSP CA証明書", 'OCSP認証局（CA）証明書が格納されるファイルの場所を指定します。 これらの証明書は、OCSPレスポンダからのレスポンスを確認するために使用されます（また、そのレスポンスが偽装されていないか、または妥協されていないことを確認してください）。 このファイルには、証明書チェーン全体が含まれている必要があります。 このファイルにルート証明書が含まれていない場合、LSWSはファイルに追加することなくシステムディレクトリのルート証明書を見つけることができますが、この検証に失敗した場合はルート証明書をこのファイルに追加してください。<br/><br/>この設定はオプションです。 この設定が設定されていない場合、サーバーは自動的に&quot;CA証明書ファイル&quot;をチェックします。', '', 'ファイル名への絶対パス又は$SERVER_ROOTからの相対パス', '');

$_tipsdb['ocspRespMaxAge'] = new DAttrHelp("OCSPの応答最大年齢（秒）", 'このオプションは、OCSP応答の許容可能な最大経過時間を設定します。 OCSP応答がこの最大年齢より古い場合、サーバーはOCSP応答者に新しい応答を要求します。 デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>86400</span>です。 この値を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-1</span>に設定すると、最大年齢を無効にすることができます。', '', '秒数', '');

$_tipsdb['ocspResponder'] = new DAttrHelp("OCSPレスポンダ", '使用するOCSPレスポンダのURLを指定します。 設定されていない場合、サーバーは認証局の発行者証明書に記載されているOCSPレスポンダに接続を試みます。 一部の発行者証明書には、OCSPレスポンダURLが指定されていない場合があります。', '', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>http://</span>で始まるURL', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>http://rapidssl-ocsp.geotrust.com</span>');

$_tipsdb['opsAuditRetainFiles'] = new DAttrHelp("アクティビティログ保持ファイル数", 'WebAdminコンソールで保持する操作監査ファイルの最大数を指定します。未設定時のデフォルト値:30ファイル。', 'より長い監査証跡が必要な場合はこの値を増やしてください。ただし、ファイル数が増えるほどディスク容量を使用することに注意してください。', '整数', '');

$_tipsdb['outBandwidth'] = new DAttrHelp("送信帯域幅（バイト/秒）", '確立された接続の数に関係なく、単一のIPアドレスへの最大の送信スループット。 実際の帯域幅は効率上の理由からこの設定よりわずかに高くなることがあります。 帯域幅は4KB単位で割り当てられます。 スロットルを無効にするには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定します。 クライアント単位の帯域幅制限（バイト/秒）は、バーチャルホストレベルの設定がサーバーレベルの設定を上回るサーバーまたはバーチャルホストレベルで設定できます。', '[パフォーマンス]パフォーマンスを向上させるため、帯域幅を8KB単位で設定します。.<br/><br/>[セキュリティ]信頼できるIPまたはサブネットワークは影響を受けません。', '整数', '');

$_tipsdb['pcKeepAliveTimeout'] = new DAttrHelp("接続キープアライブタイムアウト", 'アイドル状態の永続接続を開いたままにする最大秒数を指定します。<br/><br/>&quot;-1&quot;に設定すると、接続はタイムアウトしません。0以上に設定すると、この秒数が経過した後に接続が閉じられます。', '', '整数', '');

$_tipsdb['perClientConnLimit'] = new DAttrHelp("クライアント単位のスロットル", 'これらは、クライアントIPに基づいた接続制御設定です。 これらの設定は、DoS（サービス拒否）攻撃とDDoS（分散サービス拒否）攻撃を緩和するのに役立ちます。', '', '', '');

$_tipsdb['persistConn'] = new DAttrHelp("永続接続", 'リクエスト処理後に接続を開いたままにするかどうかを指定します。 永続接続はパフォーマンスを向上させる可能性がありますが、一部のFastCGI外部アプリケーションは永続接続を完全にはサポートしません。 デフォルトは&quot;On&quot;です。', '', 'ラジオボックスから選択', '');

$_tipsdb['phpIniOverride'] = new DAttrHelp("php.ini上書き", '現在のコンテキスト（バーチャルホストレベルまたはコンテキストレベル）でphp.ini設定を上書きするために使用します。<br/><br/>サポートされるディレクティブ：<br/>php_value<br/>php_flag<br/>php_admin_value<br/>php_admin_flag<br/><br/>その他の行/ディレクティブはすべて無視されます。', '', '上書き構文はApacheに似ており、ディレクティブと値を改行区切りで指定します。各ディレクティブには、必要に応じてphp_value、php_flag、php_admin_value、またはphp_admin_flagを付けます。', 'php_value include_path &quot;.:/usr/local/lib/php&quot;<br/>php_admin_flag engine on<br/>php_admin_value open_basedir &quot;/home&quot;');

$_tipsdb['pid'] = new DAttrHelp("PID", '現在のサーバープロセスのPID（プロセスID）。', 'PIDは、サーバーが再起動されるたびに変更されます。', '', '');

$_tipsdb['procHardLimit'] = new DAttrHelp("プロセスハードリミット", 'ソフトリミットをユーザープロセス内のハードリミットまで上げることができることを除けば、&quot;プロセスソフトリミット&quot;とほとんど同じです。 ハードリミットは、サーバー・レベルまたは個々の外部アプリケーションレベルで設定できます。 サーバーレベルの制限は、個々のアプリケーションレベルで設定されていない場合に使用されます。 オペレーティングシステムのデフォルト値は、値が両方のレベルにないか、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定されている場合に使用されます。', '', '整数', '');

$_tipsdb['procSoftLimit'] = new DAttrHelp("プロセスソフトリミット", 'ユーザーに代わって作成できるプロセスの総数を制限します。 既存のすべてのプロセスは、開始される新しいプロセスだけでなく、この限度に対してカウントされます。 制限は、サーバーレベルまたは個々の外部アプリケーションレベルで設定できます。<br/>サーバーレベルの制限は、個々のアプリケーションレベルで設定されていない場合に使用されます。 この値が0または両方のレベルにない場合、オペレーティングシステムのデフォルト設定が使用されます。', 'PHPスクリプトはプロセスをフォークするために呼び出すことができます。 この制限の主な目的は、フォーク爆弾や他のプロセスを作成するPHPプロセスによって引き起こされる他の攻撃を防ぐための最終防衛線です。<br/>この設定を低すぎると、機能が著しく損なわれる可能性があります。 この設定は特定のレベル以下では無視されます。<br/>suEXECデーモンモードを使用する場合、親プロセスが制限されないように、実際のプロセス制限はこの設定よりも高くなります。', '整数', '');

$_tipsdb['proxyContext'] = new DAttrHelp("プロキシコンテキスト", 'プロキシコンテキストは、このバーチャルホストを透過的なリバースプロキシサーバとして有効にします。 このプロキシサーバーは、HTTPプロトコルをサポートするWebサーバーまたはアプリケーションサーバーの前で実行できます。 このバーチャルホストがプロキシする外部Webサーバーは、プロキシコンテキストを設定する前に&quot;外部アプリケーション&quot;で定義されている必要があります。', '', '', '');

$_tipsdb['proxyProtocol'] = new DAttrHelp("PROXYプロトコル", 'PROXYプロトコルを使用してこのサーバーと通信するフロントエンドプロキシのIP/サブネットのリストです。設定すると、サーバーはリスト内のIP/サブネットからの受信接続にPROXYプロトコルを使用し、PROXYプロトコルを使用できない場合は通常の接続にフォールバックします。<br/>HTTP、HTTPS、HTTP2、およびwebsocket接続に適用されます。', '', 'IPアドレスまたはサブネットワークのカンマ区切りリスト。', '');

$_tipsdb['proxyWebServer'] = new DAttrHelp("Webサーバー", '外部Webサーバーの名前を指定します。 この外部Webサーバーは、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['quicBasePLPMTU'] = new DAttrHelp("PLPMTU基準値", 'QUICがデフォルトで使用するPLPMTU（ヘッダーを除く最大パケットサイズ）の最大値をバイト単位で指定します。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定すると、QUICがサイズを選択します。<br/>この設定は&quot;PLPMTU最大値&quot;より小さく設定する必要があります。<br/>デフォルト値：0', '', '0、または1200から65527までの整数', '');

$_tipsdb['quicCfcw'] = new DAttrHelp("接続フロー制御ウィンドウ", 'QUIC接続用に割り当てるバッファの初期サイズです。デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1.5M</span>です。', 'ウィンドウサイズを大きくすると、より多くのメモリを使用します。', '64Kから512Mまでの数値', '');

$_tipsdb['quicCongestionCtrl'] = new DAttrHelp("輻輳制御", '使用する輻輳制御アルゴリズムです。手動で設定するか、&quot;Default&quot;オプションを選択して使用中のQUICライブラリに任せることができます。<br/>デフォルト値：Default', '', 'ドロップダウンリストから選択', '');

$_tipsdb['quicEnable'] = new DAttrHelp("HTTP3/QUICを有効にする", 'サーバー全体でHTTP3/QUICネットワークプロトコルを有効にします。デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>です。', 'この設定が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定されている場合でも、リスナーレベルの&quot;HTTP3/QUIC(UDP)ポートを開く&quot;設定、またはバーチャルホストレベルの&quot;HTTP3/QUICを有効にする&quot;設定でHTTP3/QUICを無効にできます。', 'ラジオボックスから選択', '');

$_tipsdb['quicEnableDPLPMTUD'] = new DAttrHelp("DPLPMTUDを有効にする", 'Datagram Packetization Layer Path Maximum Transmission Unit Discovery(DPLPMTUD)を有効にします。<br/><b>   <a href="     https://tools.ietf.org/html/rfc8899   " target="_blank" rel="noopener noreferrer">     DPLPMTUDの背景（RFC 8899）   </a> </b><br/>デフォルト値：Yes', '', 'ラジオボックスから選択', '');

$_tipsdb['quicHandshakeTimeout'] = new DAttrHelp("ハンドシェイクタイムアウト（秒）", '新しいQUIC接続がハンドシェイクを完了するために与えられる秒数です。この時間を過ぎると接続は中止されます。デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>10</span>です。', '', '1から15までの整数', '');

$_tipsdb['quicIdleTimeout'] = new DAttrHelp("アイドルタイムアウト（秒）", 'アイドル状態のQUIC接続が閉じられるまでの秒数です。デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>30</span>です。', '', '10から30までの整数', '');

$_tipsdb['quicMaxCfcw'] = new DAttrHelp("最大接続フロー制御ウィンドウ", 'ウィンドウ自動調整によって接続フロー制御ウィンドウバッファが到達できる最大サイズを指定します。<br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>です。これは&quot;接続フロー制御ウィンドウ&quot;の値が使用され、自動調整が行われないことを意味します。', 'ウィンドウサイズを大きくすると、より多くのメモリを使用します。', '0、または64Kから512Mまでの数値', '');

$_tipsdb['quicMaxPLPMTU'] = new DAttrHelp("PLPMTU最大値", 'PLPMTU（ヘッダーを除く最大パケットサイズ）のプローブ上限をバイト単位で指定します。この設定は、DPLPMTUD検索範囲内の&quot;maximum packet size&quot;を制限するために使用されます。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>に設定すると、QUICがサイズを選択します（デフォルトでは、LSQUICはMTUを1,500バイト（Ethernet）と仮定します）。<br/>この設定は&quot;PLPMTU基準値&quot;より大きく設定する必要があります。<br/>デフォルト値：0', '', '0、または1200から65527までの整数', '');

$_tipsdb['quicMaxSfcw'] = new DAttrHelp("最大ストリームフロー制御ウィンドウ", 'ウィンドウ自動調整によってストリームフロー制御ウィンドウが到達できる最大サイズを指定します。<br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>です。これは&quot;ストリームフロー制御ウィンドウ&quot;の値が使用され、自動調整が行われないことを意味します。', 'ウィンドウサイズを大きくすると、より多くのメモリを使用します。', '0、または64Kから128Mまでの数値', '');

$_tipsdb['quicMaxStreams'] = new DAttrHelp("接続あたりの最大同時ストリーム数", 'QUIC接続ごとに許可される同時ストリームの最大数です。デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>100</span>です。', '', '10から1000までの整数', '');

$_tipsdb['quicSfcw'] = new DAttrHelp("ストリームフロー制御ウィンドウ", 'QUIC接続がストリームごとに受信できる初期データ量です。デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1M</span>です。', 'ウィンドウサイズを大きくすると、より多くのメモリを使用します。', '64Kから128Mまでの数値', '');

$_tipsdb['quicShmDir'] = new DAttrHelp("QUIC SHMディレクトリ", 'QUICデータを共有メモリに保存するために使用するディレクトリを指定します。<br/>デフォルトでは、サーバーのデフォルトSHMディレクトリ<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/dev/shm</span>が使用されます。', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/dev/shm</span>など、RAMベースのパーティションを推奨します。', 'パス', '');

$_tipsdb['quicVersions'] = new DAttrHelp("HTTP3/QUICバージョン", '有効にするHTTP3/QUICバージョンのリストです。この設定は、HTTP3/QUICサポートをリストされたバージョンに制限する場合にのみ使用してください。通常は空のままにするのが最適です。', '最適な設定を自動的に適用するため、この設定は空のままにすることを推奨します。', 'カンマ区切りリスト', 'h3-29, h3-Q050');

$_tipsdb['railsDefaults'] = new DAttrHelp("Rack/Rails デフォルト設定", 'Rack/Railsアプリケーションのデフォルト設定です。これらの設定はコンテキストレベルで上書きできます。', '', '', '');

$_tipsdb['rcvBufSize'] = new DAttrHelp("受信バッファサイズ（バイト）", '各TCPソケットの受信バッファーサイズ。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>512K</span>は許容されるバッファの最大サイズです。', '[パフォーマンス]オペレーティングシステムのデフォルトのバッファサイズを使用するには、この値を &quot;未設定&quot;のままにするか、0に設定することをお勧めします。<br/>[パフォーマンス]大きい受信バッファは、大きなペイロード、すなわちファイルアップロードで着信要求を処理するときのパフォーマンスを向上させます。<br/>[パフォーマンス]これを低い値に設定すると、ソケットあたりのスループットとメモリ使用量が減少し、メモリがボトルネックになった場合にサーバーがより多くの同時ソケットを持つことが可能になります。', '整数', '');

$_tipsdb['realm'] = new DAttrHelp("レルム", 'このコンテキストの認可レルムを指定します。 このコンテキストにアクセスするには、有効なユーザー名とパスワードを指定する必要があります。 &quot;認可レルム&quot;は&quot;バーチャルホストのセキュリティ&quot;セクションに設定されています。 この設定では、各レルムの&quot;レルム名&quot;が使用されます。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['realmName'] = new DAttrHelp("レルム名", '認可レルムの一意の名前を指定します。', '', 'テキスト', '');

$_tipsdb['realms'] = new DAttrHelp("認可レルム", 'このバーチャルホストのすべての認可レルムをリストします。 認可レルムは、権限のないユーザーが保護されたWebページにアクセスするのをブロックするために使用されます。 レルムは、オプションのグループ分類を持つユーザー名とパスワードを含むユーザーディレクトリです。 認可は、コンテキスト・レベルで実行されます。 異なるコンテキストは同じレルム（ユーザデータベース）を共有できるため、レルムはそれらを使用するコンテキストとは別に定義されます。 コンテキスト設定では、これらの名前でレルムを参照できます。', '', '', '');

$_tipsdb['realtimerpt'] = new DAttrHelp("リアルタイム統計", 'リアルタイム統計のリンクは、リアルタイムのサーバステータスレポートを含むページにつながります。 これは、システムを監視するのに便利なツールです。 <br/><br/>このレポートには、サーバー統計のスナップショットが表示されます。 このスナップショットのリフレッシュレートは、右上隅のリフレッシュインターバルドロップダウンリストによって制御されます。<br/><br/>このレポートには、次のセクションが含まれます： <ul><li>サーバーの正常性は、基本的なサーバー統計、稼働時間、負荷、およびanti-DDoSでブロックされたIPを表示します。</li>   <li>サーバーは、現在のトラフィックのスループット、接続、およびリクエスト統計を表示します。</li>  <li>バーチャルホストは、各バーチャルホストのリクエスト処理状況と外部アプリケーションの状態を表示します。</li>  <li>外部アプリケーションは、現在実行中の外部アプリケーションとその使用状況の統計情報を表示します。   CGIデーモンプロセスlscgidは、常に外部アプリケーションとして実行されます。</li> </ul><br/><br/>リアルタイム統計の行の多くにグラフアイコンがあります。 このアイコンをクリックすると、その行の統計がリアルタイムで更新されたグラフが開きます。<br/><br/>「サーバー」セクションの「要求」の横に、「詳細」というラベルの付いたリンクがあります。 このリンクをクリックすると、Requests Snapshotが表示されます。ここでは、どのクライアントが特定の種類のリクエストを行っているのか、サイトのどの部分がボトルネックになっているのかを詳しく見ることができます。 青色の領域のフィールドを使用すると、スナップショットをフィルタリングしてサーバーの特定の部分を分離したり、特定の処理を実行しているクライアントを探すことができます。', '', '', '');

$_tipsdb['recaptchaAllowedRobotHits'] = new DAttrHelp("許可するロボットヒット数", '「good bots」の通過を許可する10秒あたりのヒット数です。サーバーが高負荷状態の場合、ボットは引き続きスロットルされます。<br/><br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>3</span>です。', '', '整数', '');

$_tipsdb['recaptchaBotWhiteList'] = new DAttrHelp("ボット許可リスト", 'アクセスを許可するカスタムユーザーエージェントのリストです。allowedRobotHitsを含む「good bots」の制限対象になります。', '', 'ユーザーエージェントのリストを1行に1つずつ指定します。Regexを使用できます。', '');

$_tipsdb['recaptchaMaxTries'] = new DAttrHelp("最大試行回数", '最大試行回数は、訪問者を拒否する前に許可されるCAPTCHA試行の最大回数を指定します。<br/><br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>3</span>です。', '', '整数', '');

$_tipsdb['recaptchaRegConnLimit'] = new DAttrHelp("接続数上限", 'CAPTCHAを有効化するために必要な同時接続数（SSLおよび非SSL）です。同時接続数がこの数値を下回るまで、CAPTCHAは引き続き使用されます。<br/><br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>15000</span>です。', '', '整数', '');

$_tipsdb['recaptchaSecretKey'] = new DAttrHelp("シークレットキー", 'reCAPTCHAまたはhCaptchaサービスによって割り当てられる秘密キーです。<br/><br/>reCAPTCHAを使用する場合、設定されていなければデフォルトのシークレットキーが使用されます（非推奨）。', '', '', '');

$_tipsdb['recaptchaSiteKey'] = new DAttrHelp("サイトキー", 'reCAPTCHAまたはhCaptchaサービスによって割り当てられる公開キーです。<br/><br/>reCAPTCHAを使用する場合、設定されていなければデフォルトのサイトキーが使用されます（非推奨）。', '', '', '');

$_tipsdb['recaptchaSslConnLimit'] = new DAttrHelp("SSL接続数上限", 'CAPTCHAを有効化するために必要な同時SSL接続数です。同時接続数がこの数値を下回るまで、CAPTCHAは引き続き使用されます。<br/><br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>10000</span>です。', '', '整数', '');

$_tipsdb['recaptchaType'] = new DAttrHelp("CAPTCHAの種類", 'キーペアで使用するCAPTCHAの種類を指定します。<br/>キーペアが指定されておらず、この設定が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Not Set</span>に設定されている場合、CAPTCHA種類<span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Invisible</span>のデフォルトキーペアが使用されます。<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Checkbox</span>は、訪問者が検証するためのチェックボックスCAPTCHAを表示します。 （&quot;サイトキー&quot;、&quot;シークレットキー&quot;が必要）<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Invisible</span>はCAPTCHAを自動的に検証しようとし、成功した場合は目的のページにリダイレクトします。 （&quot;サイトキー&quot;、&quot;シークレットキー&quot;が必要）<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>hCaptcha</span>は、CAPTCHAプロバイダー <a href="https://www.hcaptcha.com" target="_blank" rel="noopener noreferrer">hCaptcha</a>をサポートするために使用できます。 （&quot;サイトキー&quot;、&quot;シークレットキー&quot;が必要）<br/><br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Invisible</span>です。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['recaptchaVhReqLimit'] = new DAttrHelp("同時リクエスト制限", 'CAPTCHAを有効化するために必要な同時リクエスト数です。同時リクエスト数がこの数値を下回るまで、CAPTCHAは引き続き使用されます。<br/><br/>デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>15000</span>です。', '', '整数', '');

$_tipsdb['redirectContext'] = new DAttrHelp("リダイレクトコンテキスト", 'リダイレクトコンテキストは、1つのURIまたはURIのグループを別の場所に転送するために使用できます。 宛先URIは、同じWebサイト（内部リダイレクト）または別のWebサイトを指す絶対URI（外部リダイレクト）のいずれかにすることができます。', '', '', '');

$_tipsdb['renegProtection'] = new DAttrHelp("SSL再交渉保護", 'SSL再交渉保護を有効にするかどうかを指定します。 SSLハンドシェイクベースの攻撃に対して防御します。デフォルト値は&quot;Yes&quot;です。', 'この設定は、リスナーレベルとバーチャルホストレベルで有効にできます。', 'ラジオボックスから選択', '');

$_tipsdb['required'] = new DAttrHelp("必要（許可ユーザー/グループ）", 'このコンテキストにアクセスできるユーザー/グループを指定します。 これにより、複数のコンテキストにわたって1つのユーザー/グループデータベース（&quot;レルム&quot;で指定）を使用できますが、そのデータベースの特定のユーザー/グループのみがこのコンテキストにアクセスできます。', '', '構文はApache Requireディレクティブと互換性があります。 例えば： <ul> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>user username [username ...]</span><br/>リストされたユーザーだけがこのコンテキストにアクセスできます。</li> <li> <span class=&quot;lst-inline-token lst-inline-token--value&quot;>group groupid [groupid ...]</span><br/>リストされたグループに属するユーザーのみがこのコンテキストにアクセスできます。</li> </ul> この設定を指定しないと、すべての有効なユーザーがこのリソースにアクセスできます。', '');

$_tipsdb['requiredPermissionMask'] = new DAttrHelp("必要な許可マスク", 'サーバーが提供する静的ファイルに必要なアクセス許可マスクを指定します。 たとえば、全員が読み取り可能なファイルのみを処理できる場合は、値を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0004</span>に設定します。 すべての値について<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>を参照してください', '', '8進数', '');

$_tipsdb['respBuffer'] = new DAttrHelp("レスポンスバッファリング", '外部アプリケーションから受信したレスポンスをバッファリングするかどうかを指定します。 &quot;nph-&quot;（Non-Parsed-Header）スクリプトが検出された場合、完全なHTTPヘッダーを持つレスポンスではバッファリングがオフになります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['restart'] = new DAttrHelp("変更を適用する/緩やかなリスタート", '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>緩やかな再起動</span>をクリックすると、新しいサーバープロセスが開始されます。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>緩やかな再起動</span>の場合、古いサーバープロセスは、すべてのリクエストが完了した後（または&quot;グレースフルリスタートタイムアウト（秒）&quot;に達すると）終了します。<br/><br/>設定の変更は、次回の再起動時に適用されます。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>緩やかな再起動</span>は、サーバーのダウンタイムなしにこれらの変更を適用します。', '緩やかな再起動では、通常2秒未満で新しいサーバープロセスが生成されます。', '', '');

$_tipsdb['restrained'] = new DAttrHelp("アクセス制限", 'このバーチャルホストのルート（$VH_ROOT）を超えるファイルにこのWebサイトからアクセスできるかどうかを指定します。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定すると、$VH_ROOT以下のファイルにしかアクセスできず、$VH_ROOTを超えるファイルやディレクトリを指すシンボリックリンクまたはContextへのアクセスは拒否されます。 しかし、これはCGIスクリプトのアクセシビリティを制限しません。 これは共有ホスティング環境で便利です。 &quot;シンボリックリンクをたどる&quot;は、ユーザーが$VH_ROOT内でシンボリックリンクを使用できるように<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定できます。 $VH_ROOTを超えるものはありません。', '[セキュリティ]共有ホスティング環境で有効にします。', 'ラジオボックスから選択', '');

$_tipsdb['restrictedDirPermissionMask'] = new DAttrHelp("スクリプトディレクトリのアクセス許可マスクの制限", 'サーバーが提供しないスクリプトファイルの親ディレクトリの制限付きアクセス許可マスクを指定します。 たとえば、グループおよびワールド書き込み可能なディレクトリでPHPスクリプトを処理することを禁止するには、マスクを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>022</span>に設定します。 デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>000</span>です。 このオプションを使用して、アップロードされたファイルのディレクトリ下でスクリプトを提供しないようにすることができます。<br/><br/>すべての値について<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>を参照してください。', '', '8進数', '');

$_tipsdb['restrictedPermissionMask'] = new DAttrHelp("制限付き許可マスク", 'サーバーが提供しない静的ファイルに対する制限付きのアクセス許可マスクを指定します。 たとえば、実行可能ファイルの配信を禁止するには、マスクを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0111</span>に設定します。<br/><br/>すべての値について<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>を参照してください。', '', '8進数', '');

$_tipsdb['restrictedScriptPermissionMask'] = new DAttrHelp("スクリプトのアクセス許可マスクの制限", 'サーバーが提供しないスクリプトファイルに対する制限付きアクセス許可マスクを指定します。 たとえば、グループおよびワールド書き込み可能なPHPスクリプトの配信を禁止するには、マスクを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>022</span>に設定します。 デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>000</span>です。<br/><br/>すべての値について<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>を参照してください。', '', '8進数', '');

$_tipsdb['retryTimeout'] = new DAttrHelp("リトライタイムアウト（秒）", '以前に通信問題が発生した外部アプリケーションを再試行する前に、サーバーが待機する時間を指定します。', '', '整数', '');

$_tipsdb['reusePort'] = new DAttrHelp("REUSEPORTを有効にする", 'SO_REUSEPORTソケットオプションを使用して、受信トラフィックを複数のワーカーへ分散します。 この設定はマルチワーカーライセンスでのみ有効です。有効にすると、すべてのワーカーがこのリスナーに自動的にバインドされ、 &quot;バインディング&quot;設定は無視されます。<br/><br/>デフォルト値: オン', '', 'ラジオボックスから選択', '');

$_tipsdb['rewriteBase'] = new DAttrHelp("書き換えベース", '書き換えルールのベースURLを指定します。', '', 'URL', '');

$_tipsdb['rewriteInherit'] = new DAttrHelp("継承を書き直す", '親コンテキストから書き換えルールを継承するかどうかを指定します。 書き換えが有効で継承されていない場合は、このコンテキストで定義されている書き換えのベースルールと書き換えルールが使用されます。', '', 'ラジオボタンから選択', '');

$_tipsdb['rewriteLogLevel'] = new DAttrHelp("ログレベル", '書き換えエンジンのデバッグ出力の詳細レベルを指定します。 この値の範囲は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>-<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>です。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>はロギングを無効にします。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>は最も詳細なログを生成します。 このオプションを有効にするには、サーバーとバーチャルホストのエラーログ&quot;ログレベル&quot;を少なくとも<span class=&quot;lst-inline-token lst-inline-token--value&quot;>INFO</span>以上に設定する必要があります。 これは、書き換えルールをテストする場合に便利です。', '', '整数', '');

$_tipsdb['rewriteMapLocation'] = new DAttrHelp("ロケーション", '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>MapType:MapSource</span>構文を使用して、Rewriteマップの場所を指定します。<br/>LiteSpeedのRewriteエンジンは、次の3種類のRewriteマップをサポートしています: <ul> 	<li><b>標準プレーンテキスト</b> <blockquote> 		<b>MapType:</b> txt;<br/>		<b>MapSource:</b> 有効なプレーンASCIIファイルへのファイルパス。 </blockquote> 		このファイルの各行には空白で区切られた2つの要素が含まれていなければなりません。         最初の要素はキーで、2番目の要素は値です。 コメントには先頭に「<span class=&quot;lst-inline-token lst-inline-token--value&quot;>#</span>」という記号を付けることができます。 	</li> 	<li><b>ランダム化されたプレーンテキスト</b> <blockquote> 		<b>MapType:</b> rnd;<br/>		<b>MapSource:</b> 有効なプレーンASCIIファイルのファイルパス。 </blockquote> 		ファイル形式は標準プレーンテキストファイルと似ていますが、2番目の要素には&quot;<span class=&quot;lst-inline-token lst-inline-token--value&quot;>|</span>&quot;記号で区切られた         複数の選択肢を含めることができ、Rewriteエンジンによってランダムに選択されます。 	</li> 	<li><b>内部関数</b> <blockquote> 	    <b>MapType:</b> int;<br/>		<b>MapSource:</b> 内部文字列関数 </blockquote> 		4つの関数を利用できます： 		<ul> 			<li><b>toupper:</b> 検索キーを大文字に変換します。</li> 			<li><b>tolower:</b> 検索キーを小文字に変換します。</li> 			<li><b>escape:</b> 検索キーにURLエンコードを実行します。</li> 			<li><b>unescape:</b> 検索キーでURLデコードを実行します。</li> 		</ul> 	</li> 	Apacheで利用可能な次のマップタイプはLiteSpeedでは実装されていません：<br/>ハッシュファイルと外部書き換えプログラム。 </ul> LiteSpeedのRewriteエンジン実装は、ApacheのRewriteエンジン仕様に従います。 Rewriteマップの詳細については、<a href="http://httpd.apache.org/docs/current/mod/mod_rewrite.html" target="_blank" rel="noopener noreferrer">Apacheのmod_rewriteドキュメント</a>を参照してください。', '', '文字列', '');

$_tipsdb['rewriteMapName'] = new DAttrHelp("名前", 'バーチャルホストレベルのRewriteマップに一意の名前を指定します。 この名前は、Rewriteルール内のマッピング参照で使用されます。 この名前を参照するときは、次の構文のいずれかを使用する必要があります: <blockquote><code> $\{MapName:LookupKey\}<br/>$\{MapName:LookupKey|DefaultValue\} </code></blockquote><br/>LiteSpeedのRewriteエンジン実装は、ApacheのRewriteエンジン仕様に従います。 Rewriteマップの詳細については、<a href="http://httpd.apache.org/docs/current/mod/mod_rewrite.html" target="_blank" rel="noopener noreferrer">Apacheのmod_rewriteドキュメント</a>を参照してください。', '', '文字列', '');

$_tipsdb['rewriteRules'] = new DAttrHelp("Rewriteルール", 'バーチャルホストレベルのRewriteルールのリストを指定します。<br/>ここにはドキュメントルートレベルのRewriteルールを追加しないでください。.htaccess由来のドキュメントルートレベルのRewriteルールがある場合は、代わりにURI「/」の静的コンテキストを作成し、そこにRewriteルールを追加してください。<br/>Rewriteルールは1つの<span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteRule</span>ディレクティブで構成され、任意で複数の<span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteCond</span>ディレクティブを前に置くことができます。 <ul>   <li>各ディレクティブは1行だけにしてください。</li>   <li>     <span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteCond</span>と<span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteRule</span>はApacheのRewriteディレクティブ構文に従います。Apache設定ファイルからRewriteディレクティブをコピーして貼り付けるだけで使用できます。   </li>   <li>     LiteSpeedとApache mod_rewriteの実装には小さな違いがあります:     <ul>       <li>         <span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{LA-U:variable\}</span>と<span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{LA-F:variable\}</span>はLiteSpeed Rewriteエンジンでは無視されます。       </li>       <li>         LiteSpeed Rewriteエンジンには2つの新しいサーバー変数があります。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{CURRENT_URI\}</span>はRewriteエンジンによって処理されている現在のURIを表し、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{SCRIPT_NAME\}</span>は対応するCGI環境変数と同じ意味を持ちます。       </li>       <li>         LiteSpeed Rewriteエンジンはループを避けるため、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>[L]</span>フラグに遭遇するとRewriteルールの処理を停止します。一方、Apache mod_rewriteは現在の反復についてのみRewriteルールの処理を停止します。この動作はApache mod_rewriteの<span class=&quot;lst-inline-token lst-inline-token--value&quot;>[END]</span>フラグに似ています。       </li>     </ul>   </li> </ul><br/>LiteSpeedのRewriteエンジン実装はApacheのRewriteエンジン仕様に従います。Rewriteルールの詳細については、<a href="https://httpd.apache.org/docs/current/mod/mod_rewrite.html" target="_blank" rel="noopener noreferrer">Apacheのmod_rewriteドキュメント</a>と<a href="https://httpd.apache.org/docs/current/rewrite/" target="_blank" rel="noopener noreferrer">ApacheのURL Rewriteガイド</a>を参照してください。', '', '文字列', '');

$_tipsdb['rubyBin'] = new DAttrHelp("Rubyパス", 'Ruby実行可能ファイルへのパスです。 通常はRubyのインストール先に応じて、/usr/bin/rubyまたは/usr/local/bin/rubyになります。', '', '絶対パス。', '');

$_tipsdb['runOnStartUp'] = new DAttrHelp("起動時に実行", 'サーバー起動時に外部アプリケーションを起動するかどうかを指定します。 自身の子プロセスを管理でき、&quot;インスタンス&quot;の値が&quot;1&quot;に設定されている外部アプリケーションにのみ適用されます。<br/><br/>有効にすると、外部プロセスは実行時ではなくサーバー起動時に作成されます。<br/><br/>&quot;Yes (Detached mode)&quot;を選択した場合、デタッチされたすべてのプロセスは、サーバーレベルでは$SERVER_ROOT/admin/tmp/配下、 バーチャルホストレベルでは$VH_ROOT/配下の&#039;.lsphp_restart.txt&#039;ファイルにtouchすることで再起動できます。<br/><br/>デフォルト値：Yes (Detached mode)', '[パフォーマンス] Railsアプリのように、設定された外部プロセスの起動オーバーヘッドが大きい場合は、 このオプションを有効にすると最初のページ応答時間を短縮できます。', 'ラジオボックスから選択', '');

$_tipsdb['runningAs'] = new DAttrHelp("実行ユーザ・グループ", 'サーバープロセスが実行されるユーザー/グループを指定します。 これは、インストールの前にconfigureコマンドを実行するときに、 &quot;--with-user&quot;と &quot;--with-group&quot;というパラメータを使用して設定されます。これらの値をリセットするには、configureコマンドを再実行して再インストールする必要があります。', '[セキュリティ]サーバーは、&quot;root&quot;のような特権ユーザーとして実行しないでください。 サーバーが、ログイン/シェルアクセスを持たない特権のないユーザー/グループの組み合わせで実行するように構成されていることが重要です。 一般的に<span class=&quot;lst-inline-token lst-inline-token--value&quot;>nobody</span>のユーザー/グループが良い選択です。', '', '');

$_tipsdb['scgiContext'] = new DAttrHelp("SCGIコンテキスト", 'SCGIアプリケーションは直接使用できません。SCGIアプリケーションは、スクリプトハンドラとして設定するか、SCGIコンテキストを通じてURLにマップする必要があります。SCGIコンテキストはURIをSCGIアプリケーションに関連付けます。', '', '', '');

$_tipsdb['scgiapp'] = new DAttrHelp("SCGIアプリ", 'SCGIアプリケーションの名前を指定します。このアプリケーションは、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['servAction'] = new DAttrHelp("アクション", 'このメニューからは、次の6つのアクションを使用できます：  <span class=&quot;lst-inline-token lst-inline-token--value&quot;>緩やかな再起動</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>デバッグログの切り替え</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>サーバーログビューア</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>リアルタイム統計</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>バージョンマネージャ</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>PHPをコンパイル</span>です。 <ul><li>&quot;変更を適用する/緩やかなリスタート&quot;は、処理中のリクエストを中断することなく、サーバープロセスを平滑に再起動します。</li> 	<li>&quot;デバッグログを切り替える&quot;は、デバッグログのオン/オフを切り替えます。</li> 	<li>&quot;サーバーログビューア&quot;は、ログビューアを介してサーバーログを表示できます。</li> 	<li>&quot;リアルタイム統計&quot;は、リアルタイムのサーバステータスを表示できます。</li> 	<li>&quot;バージョン管理&quot;は、LSWSの新しいバージョンをダウンロードし、異なるバージョン間で切り替えることができます。</li> 	<li>PHPをコンパイルすると、PHPをLiteSpeed Web Server用にコンパイルできます。</li> </ul>', 'シェルユーティリティ<span class=&quot;lst-inline-token lst-inline-token--command&quot;>$SERVER_ROOT/bin/lswsctrl</span>を使用してサーバープロセスを制御することもできますが、ログインシェルが必要です。', '', '');

$_tipsdb['servModules'] = new DAttrHelp("サーバーモジュール", 'サーバーモジュール構成は、モジュール構成データをグローバルに定義します。 一度定義されると、リスナーとバーチャルホストはモジュールとモジュール構成にアクセスできます。<br/><br/>処理されるすべてのモジュールは、サーバー構成に登録する必要があります。 サーバー構成では、モジュール・パラメーター・データのデフォルト値も定義されています。 これらの値は、リスナーおよびバーチャルホスト構成データによって継承または上書きできます。<br/><br/>モジュールの優先順位は、サーバーレベルでのみ定義され、リスナーとバーチャルホストの設定によって継承されます。', '', '', '');

$_tipsdb['serverName'] = new DAttrHelp("サーバー名", 'このサーバーの一意の名前です。空の場合、デフォルトでサーバーのホスト名が使用されます。', '', '', '');

$_tipsdb['serverPriority'] = new DAttrHelp("プライオリティ", 'サーバープロセスの優先度を指定します。値の範囲は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-20</span>〜<span class=&quot;lst-inline-token lst-inline-token--value&quot;>20</span>です。 数値が小さいほど優先度が高くなります。', '[パフォーマンス]通常、ビジー状態のサーバーで優先度を高くすると、Webパフォーマンスが若干高くなります。 データベースプロセスの優先度よりも高い優先度を設定しないでください。', '整数', '');

$_tipsdb['servletContext'] = new DAttrHelp("サーブレットコンテキスト", 'サーブレットは、サーブレットコンテキストを介して個別にインポートできます。 サーブレットコンテキストは、サーブレットのURIとサーブレットエンジンの名前を指定するだけです。 Webアプリケーション全体をインポートしたくない場合や、異なるサーブレットを異なる認可レルムで保護したい場合にのみ、これを使用する必要があります。 このURIには、&quot;Java Webアプリコンテキスト&quot;と同じ要件があります。', '', '', '');

$_tipsdb['servletEngine'] = new DAttrHelp("サーブレットエンジン", 'このWebアプリケーションを提供するサーブレットエンジンの名前を指定します。 サーブレットエンジンは、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['setUidMode'] = new DAttrHelp("外部アプリSet UIDモード", '外部アプリケーションプロセスにユーザーIDを設定する方法を指定します。3つの選択肢があります： <ul><li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Server UID</span>：外部アプリケーションプロセスのユーザー/グループIDをサーバーのユーザー/グループIDと同じに設定します。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI File UID</span>：実行ファイルのユーザー/グループIDに基づいて外部CGIプロセスのユーザー/グループIDを設定します。このオプションはCGIにのみ適用され、FastCGIやLSPHPには適用されません。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Doc Root UID</span>：現在のバーチャルホストのドキュメントルートのユーザー/グループIDに基づいて、外部アプリケーションプロセスのユーザー/グループIDを設定します。</li> </ul><br/><br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Server UID</span>', '[セキュリティ] 共有ホスティング環境では、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI File UID</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Doc Root UID</span>モードを使用し、あるバーチャルホストが所有するファイルに別のバーチャルホストの外部アプリケーションスクリプトからアクセスされるのを防ぐことを推奨します。', 'ドロップダウンリストから選択', '');

$_tipsdb['shHandlerName'] = new DAttrHelp("ハンドラ名", 'ハンドラタイプがLSAPI app、Web Server (Proxy)、Fast CGI、SCGI、Load balancer、Servlet Engine、またはuWSGIの場合に、スクリプトファイルを処理する外部アプリケーションの名前を指定します。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['shType'] = new DAttrHelp("ハンドラタイプ", 'これらのスクリプトファイルを処理する外部アプリケーションのタイプを指定します。使用可能なタイプは次のとおりです: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>LSAPI app</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Web Server (Proxy)</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Fast CGI</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>SCGI</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Load balancer</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Servlet Engine</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>uWSGI</span>、または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Module Handler</span>。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI</span>および<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Module Handler</span>ハンドラタイプを除き、&quot;ハンドラ名&quot;も &quot;外部アプリケーション&quot;セクションで事前に定義された外部アプリケーションに設定する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['shmDefaultDir'] = new DAttrHelp("デフォルトのSHMディレクトリ", '共有メモリのデフォルトディレクトリを指定されたパスに変更します。 ディレクトリが存在しない場合は作成されます。 特に指定のない限り、すべてのSHMデータはこのディレクトリに保存されます。', '', 'パス', '');

$_tipsdb['showVersionNumber'] = new DAttrHelp("サーバー署名", 'サーバーの署名とバージョン番号を次の場所に表示するかどうかを指定します。 レスポンスヘッダーの「Server」値。 3つのオプションがあります： <span class=&quot;lst-inline-token lst-inline-token--value&quot;>バージョンを隠す</span>に設定すると、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>LiteSpeed</span>のみが表示されます。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>バージョンを表示する</span>に設定すると、LiteSpeedとバージョン番号が表示されます。  <span class=&quot;lst-inline-token lst-inline-token--value&quot;>フルヘッダーを隠す</span>に設定すると、サーバーヘッダー全体がレスポンスヘッダーに表示されなくなります。', '[セキュリティ]サーバのバージョン番号を公開したくない場合は、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>バージョンを隠す</span>に設定します。', 'ドロップダウンリストから選択', '');

$_tipsdb['sname'] = new DAttrHelp("名前 - サーバー", 'このサーバーを識別する一意の名前。 これは一般的な設定で指定された&quot;サーバー名&quot;です。', '', '', '');

$_tipsdb['sndBufSize'] = new DAttrHelp("送信バッファサイズ（バイト）", '各TCPソケットの送信バッファサイズ。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>512K</span>は許容されるバッファの最大サイズです。', '[パフォーマンス]オペレーティングシステムのデフォルトのバッファサイズを使用するには、この値を &quot;未設定&quot;のままにするか、0に設定することをお勧めします。<br/>[パフォーマンス] Webサイトで大きな静的ファイルが使用されている場合は、送信バッファサイズを大きくしてパフォーマンスを向上させてください。<br/>[パフォーマンス]これを低い値に設定すると、ソケットあたりのスループットとメモリ使用量が減少し、メモリがボトルネックになった場合にサーバーがより多くの同時ソケットを持つことが可能になります。', '整数', '');

$_tipsdb['softLimit'] = new DAttrHelp("接続ソフトリミット", '1つのIPから許可される同時接続のソフト制限を指定します。 このソフトリミットは、&quot;猶予期間（秒）&quot;の間に&quot;接続ハードリミット&quot;以下になると一時的に超過することができますが、接続の数が限界よりも少なくなるまで、キープアライブ接続はできるだけ早く閉じられます。 &quot;猶予期間（秒）&quot;の後に接続数がまだ制限を超えている場合、そのIPは&quot;禁止期間（秒）&quot;でブロックされます。<br/><br/>例えば、ページに多数の小さなグラフが含まれている場合、ブラウザは、特にHTTP/1.0クライアントの場合、同時に多くの接続を設定しようとする可能性があります。 これらの接続を短期間で許可する必要があります。<br/><br/>HTTP/1.1クライアントは、複数の接続を設定してダウンロード速度を上げることができ、SSLにはSSL以外の接続とは別の接続が必要です。 通常のサービスに悪影響を及ぼさないように、制限値が適切に設定されていることを確認してください。 推奨される制限は、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>5</span>〜<span class=&quot;lst-inline-token lst-inline-token--value&quot;>10</span>です。', '[セキュリティ]数字が小さいほど、より明確なクライアントに対応できます。<br/>[セキュリティ]信頼できるIPまたはサブネットワークは影響を受けません。<br/>[パフォーマンス]多数の同時クライアントマシンでベンチマークテストを実行する場合は、高い値に設定します。', '整数', '');

$_tipsdb['sslCert'] = new DAttrHelp("SSL秘密鍵と証明書", 'すべてのSSLリスナーには、ペアのSSL秘密鍵とSSL証明書が必要です。 複数のSSLリスナーは、同じ鍵と証明書を共有できます。<br/>OpenSSLなどのSSLソフトウェアパッケージを使用して、SSL秘密鍵を自分で生成することができます。 SSL証明書は、VeriSignやThawteのような認証局から購入することもできます。 自分で証明書に署名することもできます。 自己署名証明書はWebブラウザから信頼されないため、重要なデータを含む公開Webサイトでは使用しないでください。 ただし、自己署名証明書は内部使用に十分適しており、 例えばLiteSpeed WebサーバーのWebAdminコンソールへのトラフィックの暗号化に使用できます。', '', '', '');

$_tipsdb['sslDefaultCiphers'] = new DAttrHelp("デフォルト暗号スイート", 'SSL証明書用のデフォルト暗号スイートです。<br/>デフォルト値：サーバー内部デフォルト（現在のベストプラクティスに基づく）', '', 'コロン区切りの暗号指定文字列。', '');

$_tipsdb['sslEnableMultiCerts'] = new DAttrHelp("複数のSSL証明書を有効にする", 'リスナー/バーチャルホストが複数のSSL証明書を設定できるようにします。 複数の証明書が有効な場合、証明書/キーは命名規則に従うことが想定されます。 証明書の名前がserver.crtの場合、その他の可能な証明書名はserver.crt.rsa、server.crt.dsa、server.crt.eccです。 「未設定」の場合、デフォルトは「いいえ」です。', '', 'ラジオボタンから選択', '');

$_tipsdb['sslOCSP'] = new DAttrHelp("OCSPステープリング", 'オンライン証明書ステータスプロトコル（OCSP）は、デジタル証明書が有効かどうかを確認するより効率的な方法です。 OCSP応答者である他のサーバーと通信して、証明書失効リスト（CRL）をチェックする代わりに証明書が有効であることを確認します。<br/>OCSPステープリングは、このプロトコルのさらなる改良であり、証明書が要求されるたびにではなく、定期的な間隔でサーバーがOCSPレスポンダを確認できるようにします。 詳細については、<a href="   http://en.wikipedia.org/wiki/OCSP_Stapling " target="_blank" rel="noopener noreferrer">   OCSP Wikipedia </a>のページをご覧ください。', '', '', '');

$_tipsdb['sslOcspProxy'] = new DAttrHelp("OCSPプロキシ", 'OCSP検証用のプロキシサーバーアドレスとして使用されるソケットアドレスです。プロキシを使用しない場合は、この設定を未設定のままにしてください。<br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>未設定</span>', '', 'ソケットアドレス', '');

$_tipsdb['sslProtocol'] = new DAttrHelp("プロトコルバージョン", 'リスナーが受け入れるSSLプロトコルを選択します。<br/><br/>選択肢には、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>SSL v3.0</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.0</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.1</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.2</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.3</span>があります。', '', 'チェックボックスから選択', '');

$_tipsdb['sslSessionCache'] = new DAttrHelp("セッションキャッシュを有効にする", 'OpenSSLのデフォルト設定を使用してセッションIDキャッシュを有効にします。バーチャルホスト設定を有効にするには、サーバーレベル設定を&quot;Yes&quot;に設定する必要があります。<br/>デフォルト値：<br/><b>サーバーレベル：</b>Yes<br/><b>VHレベル：</b>Yes', '', 'ラジオボックスから選択', '');

$_tipsdb['sslSessionCacheSize'] = new DAttrHelp("セッションキャッシュサイズ（バイト）", 'キャッシュに格納するSSLセッションIDの最大数を設定します。 デフォルトは1,000,000です。', '', '整数', '');

$_tipsdb['sslSessionCacheTimeout'] = new DAttrHelp("セッションキャッシュタイムアウト（秒）", 'この値は、再ネゴシエーションが必要な前にセッションIDがキャッシュ内で有効である期間を決定します。 デフォルトは3,600です。', '', '整数', '');

$_tipsdb['sslSessionTicketKeyFile'] = new DAttrHelp("SSLセッションチケットキーファイル", 'SSLチケットキーを管理者が作成または維持できるようにします。 ファイルの長さは48バイトでなければなりません。 このオプションを空のままにすると、ロードバランサは独自のキーセットを生成してローテーションします。<br/>重要：前方秘匿性を維持するには、<b>SSLセッションチケットの有効期間</b>秒ごとにキーを変更することを強くお勧めします。 これができない場合は、このフィールドを空のままにすることをお勧めします。', '', 'パス', '');

$_tipsdb['sslSessionTicketLifetime'] = new DAttrHelp("SSLセッションチケットの有効期間（秒）", 'この値は、再ネゴシエーションが必要となる前にセッションチケットが有効になる期間を決定します。 デフォルトは3,600です。', '', '整数', '');

$_tipsdb['sslSessionTickets'] = new DAttrHelp("セッションチケットを有効にする", 'セッションチケットを有効にします。 「未設定」の場合、サーバーはopenSSLのデフォルトチケットを使用します。', '', 'ラジオボタンから選択', '');

$_tipsdb['sslStrictSni'] = new DAttrHelp("厳格なSNI証明書", '専用のバーチャルホスト証明書設定を厳格に要求するかどうかを指定します。有効にすると、専用の証明書設定がないバーチャルホストへのSSL接続は、デフォルトの包括証明書を使用する代わりに失敗します。<br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '', 'ラジオボックスから選択', '');

$_tipsdb['sslStrongDhKey'] = new DAttrHelp("SSL強力なDHキー", 'SSLハンドシェイクに2048または1024ビットのDHキーを使用するかどうかを指定します。 「Yes」に設定すると、2048ビットのSSLキーと証明書に2048ビットのDHキーが使用されます。 他の状況でも1024ビットのDHキーが引き続き使用されます。 デフォルトは「はい」です。<br/>以前のバージョンのJavaでは、1024ビット以上のDHキーサイズはサポートされていません。 Javaクライアントの互換性が必要な場合は、これを「いいえ」に設定する必要があります。', '', 'ラジオボタンから選択', '');

$_tipsdb['statDir'] = new DAttrHelp("統計出力ディレクトリ", 'Real-Time Statsレポートファイルが書き込まれるディレクトリを指定します。 デフォルトのディレクトリは<b>/tmp/lshttpd/</b>です。', 'サーバーの操作中に.rtreportファイルが1秒ごとに書き込まれます。 不必要なディスク書き込みを避けるには、これをRAMディスクに設定します。<br/>.rtreportファイルはサードパーティ製の監視ソフトウェアと一緒に使用してサーバーの状態を追跡できます。', '絶対パス。', '');

$_tipsdb['staticReqPerSec'] = new DAttrHelp("静的リクエスト/秒", '確立された接続の数に関係なく、1秒間に処理できる単一のIPアドレスからの静的コンテンツへの要求の最大数を指定します。<br/><br/>この制限に達すると、将来のすべての要求は次の秒までタールピットされます。 動的に生成されるコンテンツのリクエスト制限は、この制限とは関係ありません。 クライアントごとの要求制限は、サーバーまたはバーチャルホストレベルで設定できます。 バーチャルホストレベルの設定は、サーバーレベルの設定よりも優先されます。', '[セキュリティ]信頼できるIPまたはサブネットワークは影響を受けません。', '整数', '');

$_tipsdb['statuscode'] = new DAttrHelp("ステータスコード", '外部リダイレクトの応答ステータスコードを指定します。 ステータスコードが300〜399の場合、&quot;転送先URI&quot;を指定できます。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['suexecGroup'] = new DAttrHelp("suEXECグループ", '現在のコンテキストレベルで、このグループとして実行します。 <b>suEXECグループ</b>を有効にするには、バーチャルホストレベルの<b>suEXECユーザー</b>、または外部アプリケーションレベルの<b>実行ユーザー</b>を設定する必要があります。<br/><br/>この設定は、外部アプリケーションレベルの<b>実行グループ</b>設定で上書きできます。<br/><br/>デフォルト値：<b>suEXECユーザー</b>設定値', '', '有効なグループ名またはuid', '');

$_tipsdb['suexecUser'] = new DAttrHelp("suEXECユーザー", '現在のコンテキストレベルで、このユーザーとして実行します。 設定すると、この値はバーチャルホストレベルの<b>外部アプリSet UIDモード</b>設定を上書きします。<br/><br/>この設定は、外部アプリケーションレベルの<b>実行ユーザー</b>設定で上書きできます。<br/><br/>デフォルト値：未設定', '', '有効なユーザー名またはuid。', '');

$_tipsdb['suffix'] = new DAttrHelp("サフィックス", 'このスクリプトハンドラによって処理されるスクリプトファイルのサフィックスを指定します。 接尾辞は一意である必要があります。', 'サーバーは、リスト内の最初のサフィックスに特殊なMIMEタイプ（&quot;application/x-httpd-[suffix]&quot;）を自動的に追加します。 たとえば、サフィックス&quot;php53&quot;にはMIMEタイプ&quot;application/x-httpd-php53&quot;が追加されます。 最初以降のサフィックスは&quot;MIME設定&quot;設定で構成する必要があります。<br/>このフィールドではサフィックスを列挙しますが、スクリプトハンドラはサフィックスではなくMIMEタイプを使用して処理対象のスクリプトを決定します。<br/> 本当に必要なサフィックスのみを指定してください。', 'カンマ区切りリストでピリオド&quot;.&quot;は禁止されています。', '');

$_tipsdb['swappingDir'] = new DAttrHelp("スワッピングディレクトリ", 'スワッピングファイルを配置するディレクトリを指定します。サーバーがchrootモードで起動されている場合、 このディレクトリは新しいルートディレクトリからの相対パスになります。それ以外の場合は、実際のルートディレクトリからの相対パスになります。<br/><br/>サーバーは独自のバーチャルメモリを使用してシステムメモリ使用量を削減します。 バーチャルメモリとディスクスワッピングは、大きなリクエストボディと動的に生成されたレスポンスを保存するために使用されます。 スワッピングディレクトリは十分な空き容量があるディスク上に配置してください。<br/><br/>デフォルト値：/tmp/lshttpd/swap', '[パフォーマンス] スワッピングをなくすには、スワッピングディレクトリを別のディスクに配置するか、最大I/Oバッファサイズを増やします。', '絶対パス', '');

$_tipsdb['templateFile'] = new DAttrHelp("テンプレートファイル", 'このテンプレートの設定ファイルへのパスを指定します。 ファイルは$SERVER_ROOT/conf/templates/内にあり、ファイル名は&quot;.conf&quot;である必要があります。 指定したファイルが存在しない場合、テンプレートを保存しようとすると&quot;クリックして作成&quot;リンクを含むエラーが表示されます。このリンクにより、新しい空のテンプレートファイルが生成されます。テンプレートを削除すると、エントリは設定から削除されますが、実際のテンプレート設定ファイルは削除されません。', '', 'パス', '');

$_tipsdb['templateName'] = new DAttrHelp("テンプレート名", 'テンプレートの一意の名前。', '', 'テキスト', '');

$_tipsdb['templateVHAliases'] = new DAttrHelp("ドメインエイリアス", 'バーチャルホストの代替名を指定します。 すべての可能なホスト名とIPアドレスをこのリストに追加する必要があります。 ワイルドカード文字<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>と<span class=&quot;lst-inline-token lst-inline-token--value&quot;>?</span>は名前に使用できます。 ポート80以外のWebサイトには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>:<port></span>を追加します。<br/><br/>エイリアスは、以下の状況で使用されます: <ol>   <li>要求を処理するときにホストヘッダー内のホスト名を照合する。</li>   <li>FrontPageやAWstatsなどのアドオンのドメイン名/エイリアス設定を入力する。</li>   <li>バーチャルホストテンプレートに基づいてリスナーからバーチャルホストへのマッピングを構成する。</li> </ol>', '', 'ドメイン名のカンマ区切りリスト。', '');

$_tipsdb['templateVHConfigFile'] = new DAttrHelp("インスタンス化されたVHost設定ファイル", 'メンバーバーチャルホストをインスタンス化するときに生成される設定ファイルの場所を指定します。 各バーチャルホストが独自のファイルを持つように、パスには変数<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>を含める必要があります。 $SERVER_ROOT/conf/vhosts/にある必要があります。 この設定ファイルは、メンバーバーチャルホストをインスタンス化によってテンプレートから移動した後にのみ作成されます。', '管理しやすいように$VH_NAME/vhconf.confを推奨します。', '$VH_NAME変数と.confサフィックスを含む文字列', '');

$_tipsdb['templateVHDocRoot'] = new DAttrHelp("ドキュメントルート", '各メンバーバーチャルホストのドキュメントルートの一意のパスを指定します。 各メンバーバーチャルホストが独自のドキュメントルートを持つように、パスには変数<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT</span>を含める必要があります。', '', '$VH_NAMEまたは$VH_ROOT変数を含むパス', '$VH_ROOT/public_html/または$SERVER_ROOT/$VH_NAME/public_html。');

$_tipsdb['templateVHDomain'] = new DAttrHelp("ドメイン名", 'このメンバーバーチャルホストのメインドメイン名を指定します。 空白のままにすると、バーチャルホスト名が使用されます。 これは完全修飾ドメイン名である必要がありますが、IPアドレスも使用できます。 ポート80以外のWebサイトには、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>:<port></span>を追加することをお勧めします。 ドメイン名を含む設定の場合、このドメインは変数<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_DOMAIN</span>で参照できます。<br/><br/>このドメイン名は、以下の状況で使用されます: <ol>   <li>要求を処理する際にホストヘッダー内のホスト名を照合する。</li>   <li>FrontPageやAWstatsなどのアドオンのドメイン名設定を入力する。</li>   <li>バーチャルホストテンプレートに基づいてリスナーからバーチャルホストへのマッピングを構成する。</li> </ol>', '', 'ドメイン名', '');

$_tipsdb['templateVHName'] = new DAttrHelp("バーチャルホスト名", 'このバーチャルホストの一意の名前。 この名前は、すべてのテンプレートメンバーバーチャルホストとスタンドアロンバーチャルホストで一意でなければなりません。 ディレクトリパス設定では、この名前は変数<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>によって参照できます。<br/><br/>同じ名前のスタンドアロンバーチャルホストも設定されている場合、メンバーバーチャルホスト設定は無視されます。', '', 'テキスト', '');

$_tipsdb['templateVHRoot'] = new DAttrHelp("デフォルトのバーチャルホストルート", 'このテンプレートを使用して、メンバーバーチャルホストのデフォルトルートディレクトリを指定します。 変数<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>はパスに指定する必要があります。 これにより、各メンバーテンプレートに名前に基づいて別々のルートディレクトリが自動的に割り当てられます。', '', 'パス', '');

$_tipsdb['throttleBlockWindow'] = new DAttrHelp("初期ブロック期間(秒)", '許可された最大ログイン失敗回数に達した後、クライアントをブロックする初期時間を秒単位で指定します。未設定時のデフォルト値:900秒。', '正当なユーザーに過度なロックアウト時間を発生させずに、繰り返し攻撃を抑止できる十分な長さを使用してください。', '整数', '');

$_tipsdb['throttleEnabled'] = new DAttrHelp("ログインスロットルを有効にする", 'WebAdminコンソールのログインスロットルを有効にします。有効にすると、失敗したログイン試行の繰り返しが追跡され、ブルートフォースパスワード攻撃を減らすために一時的にブロックされます。このオプションのみを有効にし、他のスロットル設定を未設定のままにした場合は、組み込みのデフォルト値が使用されます。', '[セキュリティ] ログイン問題のトラブルシューティング中でない限り、本番環境では有効のままにしてください。カスタム値なしで有効にした場合、デフォルトは5回の失敗、900秒の初期ブロック、14400秒の最大ブロックです。', 'ラジオボックスから選択', '');

$_tipsdb['throttleMaxBackoff'] = new DAttrHelp("最大ブロック期間(秒)", 'ログイン失敗の繰り返しが続き、スロットルのバックオフが増加した場合の最大ブロック期間を秒単位で指定します。未設定時のデフォルト値:14400秒。', '自動化された攻撃を十分に遅らせつつ、妥当な時間内に復旧できる上限を設定してください。', '整数', '');

$_tipsdb['throttleMaxFailures'] = new DAttrHelp("最大ログイン失敗回数", 'クライアントがブロックされる前に許可される連続ログイン失敗回数を指定します。未設定時のデフォルト値:5。', '[セキュリティ] 値を低くすると保護は強くなりますが、正当なユーザーがより早くブロックされる可能性があります。', '整数', '');

$_tipsdb['toggleDebugLog'] = new DAttrHelp("デバッグログを切り替える", 'デバッグログの切り替えは、&quot;デバッグレベル&quot;の値を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>と<span class=&quot;lst-inline-token lst-inline-token--value&quot;>HIGH</span>の間で切り替えます。 デバッグログはパフォーマンスに影響し、ハードドライブをすぐに埋める可能性があるため、通常は本番サーバーで&quot;デバッグレベル&quot;を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>に設定する必要があります。 この機能を代わりに使用して、本番サーバー上の問題をデバッグするために、デバッグログを素早くオン/オフすることができます。 このようにデバッグロギングをオンまたはオフにしても、サーバー構成に表示されるものは変更されません。', '&quot;デバッグログを切り替える&quot;は、&quot;ログレベル&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>に設定されている場合にのみ機能します。<br/>[パフォーマンス] 重要！デバッグログには、各リクエストとレスポンスの詳細情報が含まれます。 有効なデバッグログはサービス性能を著しく低下させ、非常に短時間でディスク容量を使い切る可能性があります。 この機能は、サーバー問題を診断する短時間だけ使用してください。', '', '');

$_tipsdb['totalInMemCacheSize'] = new DAttrHelp("小ファイルキャッシュサイズ合計（バイト）", '小さな静的ファイルをキャッシュ/提供するためにバッファーキャッシュに割り振ることができる合計メモリーを指定します。', '', '整数', '');

$_tipsdb['totalMMapCacheSize'] = new DAttrHelp("MMAPキャッシュサイズ合計（バイト）", '中規模の静的ファイルをキャッシュ/配信するためにメモリマップされたキャッシュに割り当てることができる合計メモリを指定します。', '', '整数', '');

$_tipsdb['tp_vhFileName'] = new DAttrHelp("テンプレートで使用するファイル名", 'テンプレートベースのバーチャルホスト設定で使用するファイルパスを指定します。 各メンバーバーチャルホストが独自のファイルを参照するように、パスには変数<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT</span>を含める必要があります。 この設定はログファイルなどテンプレートが所有するファイル用であり、インスタンス化されたVHost設定ファイル用ではありません。', '', '$VH_NAMEまたは$VH_ROOT変数を含むパス', '');

$_tipsdb['umask'] = new DAttrHelp("umask", 'CGIプロセスのデフォルトのumaskを設定します。 詳細は、<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 umask</span>を参照してください。 これは、外部アプリケーション&quot;umask&quot;のデフォルト値としても機能します。', '', '有効範囲値[000]〜[777]。', '');

$_tipsdb['uploadPassByPath'] = new DAttrHelp("ファイルパスによるアップロードデータの転送", 'ファイルデータをパスでアップロードするかどうかを指定します。 有効にすると、アップロード時にファイル自体ではなく、バックエンドハンドラにファイルパスと他の情報が送信されます。 これにより、CPUリソースとファイル転送時間が節約されますが、実装するためにバックエンドに若干の更新が必要です。 無効にすると、ファイルの内容はバックエンドハンドラに転送され、要求本文は引き続きファイルに解析されます。', '[パフォーマンス]下位互換性が問題にならないようにするには、これを有効にしてファイルのアップロード処理を高速化します。', 'ラジオボタンから選択', '');

$_tipsdb['uploadTmpDir'] = new DAttrHelp("一時ファイルパス", '要求本体パーサーが動作している間に、サーバーにアップロードされるファイルが格納される一時ディレクトリ。 デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/tmp/lshttpd/</span>です。', '', '絶対パス、または$SERVER_ROOTで始まるパス（ServerおよびVHostレベル）、または$VH_ROOTで始まるパス（VHostレベル）。', '');

$_tipsdb['uploadTmpFilePermission'] = new DAttrHelp("一時ファイルのアクセス許可", '一時ディレクトリに格納されるファイルに使用するファイル権限を決定します。 サーバーレベルの設定はグローバルで、VHostレベルでオーバーライドできます。', '', '3桁の8進数。デフォルト値は666です。', '');

$_tipsdb['uri'] = new DAttrHelp("URI", 'このコンテキストのURIを指定します。 URIは &quot;/&quot;で始まる必要があります。 URIが「/」で終わる場合、このコンテキストはこのURIの下にすべてのサブURIを含みます。', '', 'URI', '');

$_tipsdb['useAIO'] = new DAttrHelp("非同期I/O (AIO)を使用する", '静的ファイルを提供するために非同期I/Oを使用するかどうか、および使用するAIO実装を指定します。オプション<span class=&quot;lst-inline-token lst-inline-token--value&quot;>LINUX AIO</span>と<span class=&quot;lst-inline-token lst-inline-token--value&quot;>io_uring</span>はLinuxマシンでのみ使用できます。<br/>デフォルト値：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '[パフォーマンス] I/O待ち時間が長いサーバーでは、AIOがパフォーマンス向上に役立つ場合があります。<br/>[注意] <span class=&quot;lst-inline-token lst-inline-token--value&quot;>io_uring</span>が選択されていても現在のマシンでサポートされていない場合、代わりに<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Linux AIO</span>が使用されます。', 'ドロップダウンリストから選択', '');

$_tipsdb['useIpInProxyHeader'] = new DAttrHelp("ヘッダー内のクライアントIPを使用", '&quot;X-Forwarded-For&quot;HTTPリクエストヘッダーに記載された最初の有効なIPアドレスを、 接続/帯域幅スロットリング、アクセス制御、IPジオロケーションなど、すべてのIPアドレス関連機能に使用するかどうかを指定します。<br/><br/>この機能は、Webサーバーがロードバランサまたはプロキシサーバーの背後にある場合に便利です。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Trusted IP Only</span>を選択すると、サーバーレベルの&quot;許可リスト&quot;で定義された信頼済みIPからリクエストが来た場合にのみ、 X-Forwarded-For IPが使用されます。<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Keep Header from Trusted IP</span>は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Trusted IP Only</span>と同じですが、バックエンドで使用されるX-Forwarded-Forヘッダーは、 接続元ピアアドレスを含めるようには変更されません。<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Use Last IP (for AWS ELB)</span>は、&quot;X-Forwarded-For&quot;リスト内の最後のIPアドレスを使用します。 AWS Elastic Load Balancerを使用している場合、または実IPが&quot;X-Forwarded-For&quot;リストの末尾に追加されることを想定している場合は、このオプションを選択してください。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['useSendfile'] = new DAttrHelp("sendfile（）を使用する", '静的ファイルを提供するためにsendfile（）システムコールを使用するかどうかを指定します。 静的ファイルは、メモリバッファキャッシュ、メモリマップキャッシュ、プレーンリード/ライト、およびsendfile（）の4つの方法で提供できます。 &quot;最大キャッシュサイズの小さいファイルサイズ（バイト）&quot;より小さいファイルはメモリバッファキャッシュから提供されます。 &quot;最大キャッシュサイズの小さいファイルサイズ（バイト）&quot;より大きいが、&quot;最大MMAPファイルサイズ（バイト）&quot;より小さいファイルは、メモリマップドキャッシュから提供されます。 &quot;最大MMAPファイルサイズ（バイト）&quot;より大きいファイルは、プレーンな読み取り/書き込みまたはsendfile（）で処理されます。 Sendfile（）は、非常に大きな静的ファイルを処理するときにCPU使用率を大幅に下げることができる「ゼロコピー」システムコールです。 Sendfile（）は最適化されたネットワークカードカーネルドライバを必要とするため、一部の小規模ベンダーのネットワークアダプタには適していない可能性があります。', '', 'ラジオボタンから選択', '');

$_tipsdb['userDBCacheTimeout'] = new DAttrHelp("ユーザーDBキャッシュタイムアウト（秒）", 'バックエンドユーザーデータベースの変更の確認頻度を指定します。 キャッシュ内のすべてのエントリにタイムスタンプがあります。 キャッシュされたデータが指定されたタイムアウトよりも古い場合、バックエンドデータベースの変更がチェックされます。 変更がなければ、タイムスタンプは現在の時刻にリセットされ、そうでない場合は新しいデータがロードされます。 サーバーのリロードとgraceful restartにより、キャッシュは直ちにクリアされます。', '[パフォーマンス]バックエンドデータベースが頻繁に変更されない場合は、パフォーマンスを向上させるために、より長いタイムアウトを設定します。', '整数', '');

$_tipsdb['userDBLocation'] = new DAttrHelp("ユーザーDBの場所", 'ユーザーデータベースの場所を指定します。データベースは$SERVER_ROOT/conf/vhosts/$VH_NAME/ディレクトリの下に保存することをお勧めします。<br/><br/>DBタイプが<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Password File</span>の場合、ユーザー/パスワード定義を含むフラットファイルへのパスです。ファイル名をクリックすると、WebAdminコンソールからこのファイルを編集できます。<br/><br/>ユーザーファイルの各行には、ユーザー名、コロン、crypt()で暗号化されたパスワードが続き、任意でコロンとユーザーが所属するグループ名が続きます。グループ名はカンマで区切ります。ユーザーデータベースにグループ情報が指定されている場合、グループデータベースはチェックされません。<br/><br/>例:<blockquote><code>john:HZ.U8kgjnMOHo:admin,user</code></blockquote>', '', 'ユーザーDBファイルへのパス。', '');

$_tipsdb['userDBMaxCacheSize'] = new DAttrHelp("ユーザーDB最大キャッシュサイズ", 'ユーザーデータベースの最大キャッシュサイズを指定します。 最近アクセスされたユーザー認証データは、最大のパフォーマンスを提供するためにメモリにキャッシュされます。', '[パフォーマンス]キャッシュが大きくなるとメモリが消費されるため、値が高くなるほどパフォーマンスが向上する場合があります。 ユーザーのデータベースサイズとサイトの使用状況に応じて適切なサイズに設定します。', '整数', '');

$_tipsdb['uwsgiContext'] = new DAttrHelp("uWSGIコンテキスト", 'uWSGIアプリケーションは直接使用できません。uWSGIアプリケーションは、スクリプトハンドラとして設定するか、uWSGIコンテキストを通じてURLにマップする必要があります。uWSGIコンテキストはURIをuWSGIアプリケーションに関連付けます。', '', '', '');

$_tipsdb['uwsgiapp'] = new DAttrHelp("uWSGIアプリ", 'uWSGIアプリケーションの名前を指定します。このアプリケーションは、サーバーまたはバーチャルホストレベルの&quot;外部アプリケーション&quot;セクションで定義する必要があります。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['vaction'] = new DAttrHelp("アクション - バーチャルホスト", 'このフィールドには、バーチャルホストを無効化、有効化、または再起動するためのボタンが表示されます。 1つのバーチャルホストで実行されたアクションは、残りのWebサーバーには影響しません。', 'バーチャルホストをコンテンツを更新するときに一時的に無効にすることをお勧めします。', '', '');

$_tipsdb['vdisable'] = new DAttrHelp("無効", '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>無効</span>アクションは、実行中のバーチャルホストを停止します。 新しいリクエストは受け付けられませんが、処理中のリクエストは通常通り終了します。', '', '', '');

$_tipsdb['venable'] = new DAttrHelp("有効", '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>有効</span>アクションは、停止したバーチャルホストを起動します。 これにより、新しい要求を受け入れることができます。', '', '', '');

$_tipsdb['verifyDepth'] = new DAttrHelp("検証の深さ", ' クライアントに有効な証明書がないと判断する前に、証明書の検証の深さを指定します。 デフォルトは &quot;1&quot;です。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['vhEnableBr'] = new DAttrHelp("Brotli圧縮を有効にする", 'このバーチャルホストでBrotli圧縮を有効にするかどうかを指定します。 この設定は、サーバーレベルで&quot;Brotli圧縮レベル(静的ファイル)&quot;がゼロ以外の値に設定されている場合にのみ有効です。', '', 'ラジオボックスから選択', '');

$_tipsdb['vhEnableGzip'] = new DAttrHelp("圧縮を有効にする", 'このバーチャルホストでGZIP圧縮を有効にするかどうかを指定します。 この設定は、サーバーレベルで&quot;圧縮を有効にする&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定されている場合にのみ有効です。', '', 'ラジオボックスから選択', '');

$_tipsdb['vhEnableQuic'] = new DAttrHelp("HTTP3/QUICを有効にする", 'このバーチャルホストでHTTP3/QUICネットワークプロトコルを有効にします。 この設定を有効にするには、サーバーレベルの&quot;HTTP3/QUICを有効にする&quot;とリスナーレベルの&quot;HTTP3/QUIC(UDP)ポートを開く&quot;もそれぞれ<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>に設定する必要があります。 デフォルト値は<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>です。', 'この設定を<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>にすると、HTTP3/QUIC広告は送信されなくなります。ブラウザにキャッシュされたHTTP3/QUIC情報が残っており、サーバーレベルとリスナーレベルでHTTP3/QUICが引き続き有効な場合、その情報のキャッシュが消えるかHTTP3/QUICプロトコルエラーが発生するまで、HTTP3/QUIC接続は引き続き使用されます。', 'ラジオボックスから選択', '');

$_tipsdb['vhMaxKeepAliveReq'] = new DAttrHelp("最大Keep-Aliveリクエスト数", 'Keep-Alive（永続）接続で処理できる最大リクエスト数を指定します。 この制限に達すると接続が閉じられます。 バーチャルホストごとに異なる制限を指定できます。 この数値は、サーバーレベルの&quot;最大キープアライブ要求&quot;の制限を超えることはできません。', '[パフォーマンス]合理的に高い値に設定します。 「1」以下の値を指定すると、キープアライブ接続が無効になります。', '整数', '');

$_tipsdb['vhModuleUrlFilters'] = new DAttrHelp("バーチャルホストモジュールのコンテキスト", 'バーチャルホストコンテキストのモジュール設定をカスタマイズするための集中管理された場所です。 コンテキストURIの設定は、バーチャルホストまたはサーバーレベルの設定を上書きします。', '', '', '');

$_tipsdb['vhModules'] = new DAttrHelp("バーチャルホストモジュール", 'バーチャルホストモジュール構成データは、デフォルトでサーバーモジュール設定から継承されます。 バーチャルホストモジュールはHTTPレベルのフックに制限されています。', '', '', '');

$_tipsdb['vhName'] = new DAttrHelp("バーチャルホスト名", 'バーチャルホストの一意の名前。 バーチャルホストのドメイン名をバーチャルホスト名として使用することをお勧めします。 バーチャルホスト名は、変数$VH_NAMEを使用して参照できます。', '', 'テキスト', '');

$_tipsdb['vhRoot'] = new DAttrHelp("バーチャルホストルート", 'バーチャルホストのルートディレクトリを指定します。 注：これはドキュメントルートでは<b>ありません</b>。このディレクトリの下に、バーチャルホストに関連するすべてのファイル（ログファイル、HTMLファイル、CGIスクリプトなど）を置くことをお勧めします。 バーチャルホストルートは、変数$VH_ROOTを使用して参照できます。', '[パフォーマンス] 異なるバーチャルホストは別々のハードドライブに配置してください。', '絶対パス又は$SERVER_ROOTからの相対パス。', '');

$_tipsdb['vhaccessLog_fileName'] = new DAttrHelp("ファイル名", 'アクセスログファイル名。', ' アクセスログファイルを別のディスクに配置します。', 'ファイル名への絶対パス、または$SERVER_ROOT、$VH_ROOTからの相対パス。', '');

$_tipsdb['vhadminEmails'] = new DAttrHelp("管理者用電子メール", 'このバーチャルホストの管理者の電子メールアドレスを指定します。', '', '電子メールアドレスのカンマ区切りリスト', '');

$_tipsdb['vhlog_fileName'] = new DAttrHelp("ファイル名", 'ログファイルのパスを指定します。', ' ログファイルは別のディスクに配置してください。', 'ファイル名への絶対パス又は$SERVER_ROOT、$VH_ROOTからの相対パス。', '');

$_tipsdb['vhlog_logLevel'] = new DAttrHelp("ログレベル", 'ロギングのレベルを指定します。 使用可能なレベルは、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ERROR</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>WARNING</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NOTICE</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>INFO</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>です。 現在の設定以上のレベルのメッセージのみが記録されます。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>に設定する場合は、サーバーログレベルも<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>に設定する必要があります。 デバッグのレベルは、&quot;デバッグレベル&quot;によってサーバーレベルでのみ制御されます。', ' &quot;デバッグレベル&quot;が<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>以外のレベルに設定されていない限り、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>ログレベルはパフォーマンスに影響を及ぼさず、推奨されます。', '選択', '');

$_tipsdb['viewlog'] = new DAttrHelp("サーバーログビューア", 'サーバーログビューアは、現在のサーバーログを参照してエラーや問題を確認するための便利なツールです。 ログビューアーは、指定されたログレベルのブロック単位でサーバーログファイルを検索します。<br/><br/>デフォルトのブロックサイズは20KBです。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Begin</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>End</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Next</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Prev</span>ボタンを使用して、大きなログファイルをナビゲートできます。', '動的に生成されるページのサイズは、&quot;最大動的レスポンスボディサイズ(バイト)&quot;によって制限されます。 したがって、ブロックが大きすぎると、ページが切り捨てられることがあります。', '', '');

$_tipsdb['virtualHostMapping'] = new DAttrHelp("バーチャルホストマッピング", 'リスナーとバーチャルホストの関係を指定します。 リスナーとバーチャルホストはドメイン名によって関連付けられています。 HTTP要求は、一致するドメイン名を持つバーチャルホストにルーティングされます。 1つのリスナーは、異なるドメイン名の複数のバーチャルホストにマップできます。 1つのバーチャルホストを異なるリスナーからマップすることもできます。 1つのリスナーは、ドメイン名の値&quot;*&quot;を持つ1つのcatchallバーチャルホストを許可できます。 リスナーのマッピングに明示的に一致するドメイン名がない場合、 リスナーはそのリクエストをそのcatchallバーチャルホストに転送します。', '[パフォーマンス] 必要なマッピングのみを追加します。 リスナーが1つのバーチャルホストにのみマッピングされている場合は、catchallマッピング&quot;*&quot;のみを設定します。', '', '');

$_tipsdb['virtualHostName'] = new DAttrHelp("バーチャルホスト", 'バーチャルホストの名前を指定します。', '', 'ドロップダウンリストから選択', '');

$_tipsdb['vname'] = new DAttrHelp("名前 - バーチャルホスト", 'このバーチャルホストを識別する一意の名前。 これは、このバーチャルホストを設定するときに指定した&quot;バーチャルホスト名&quot;です。', '', '', '');

$_tipsdb['vreload'] = new DAttrHelp("再起動 - バーチャルホスト", 'Restartアクションにより、Webサーバーはこのバーチャルホストの最新設定をロードします。 処理中のリクエストは古い設定で終了します。 新しい設定は新しいリクエストにのみ適用されます。 バーチャルホストへのすべての変更は、この方法で即時に適用できます。', '', '', '');

$_tipsdb['vstatus'] = new DAttrHelp("ステータス - バーチャルホスト", 'バーチャルホストの現在のステータス。   ステータスは<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Stopped</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Restart Required</span>、または<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running - Removed from Configuration</span>です。  <ul>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running</span>は、バーチャルホストがロードされ、サービス中であることを意味します。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Stopped</span>は、バーチャルホストがロードされていてもサービスされていない（無効になっている）ことを意味します。 </li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Restart Required</span>は、これが新しく追加されたバーチャルホストであり、サーバがまだ設定をロードしていないことを意味します。 </li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running - Removed from Configuration</span>は、バーチャルホストがサーバの設定から削除されたが、まだ稼働中であることを意味します。 </li> </ul>', '', '', '');

$_tipsdb['wsaddr'] = new DAttrHelp("アドレス", 'WebSocketバックエンドによって使用される一意のソケットアドレス。 IPv4ソケット、IPv6ソケット、Unixドメインソケット（UDS）がサポートされています。 IPv4およびIPv6ソケットは、ネットワークを介した通信に使用できます。 UDSは、WebSocketバックエンドがサーバーと同じマシンに存在する場合にのみ使用できます。', ' WebSocketバックエンドが同じマシン上で実行される場合は、UDSの使用を推奨します。 IPv4またはIPv6ソケットを使用する必要がある場合は、IPアドレスをlocalhostまたは127.0.0.1に設定し、 他のマシンからWebSocketバックエンドへアクセスできないようにします。<br/> Unixドメインソケットは、一般にIPv4またはIPv6ソケットよりも高いパフォーマンスを提供します。', 'IPv4/IPv6アドレス:ポート、UDS://パス、またはunix:パス', '127.0.0.1:5434<br/>UDS://tmp/lshttpd/php.sock<br/>unix:/tmp/lshttpd/php.sock');

$_tipsdb['wsgiBin'] = new DAttrHelp("WSGIパス", 'LiteSpeed Python Web Server Gateway Interface実行ファイル（lswsgi）へのパスです。<br/><br/>この実行ファイルは、LiteSpeedのWSGI LSAPIモジュールを使用してPythonをコンパイルすることで作成されます。', '', '絶対パス', '');

$_tipsdb['wsgiDefaults'] = new DAttrHelp("Python WSGIデフォルト設定", 'Python WSGIアプリケーションのデフォルト設定です。これらの設定はコンテキストレベルで上書きできます。', '', '', '');

$_tipsdb['wsuri'] = new DAttrHelp("URI", 'このWebSocketバックエンドを使用するURIを指定します。このURIへのトラフィックは、WebSocketアップグレードリクエストを含む場合にのみWebSocketバックエンドへ転送されます。<br/><br/>このアップグレードリクエストを含まないトラフィックは、このURIが属するコンテキストへ自動的に転送されます。このURIにコンテキストが存在しない場合、LSWSはこのトラフィックを<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$DOC_ROOT/URI</span>の場所にある静的コンテキストへアクセスしているものとして扱います。', '', 'プレーンURI(&quot;/&quot;で始まる)。URIが&quot;/&quot;で終わる場合、このWebSocketバックエンドにはこのURI配下のすべてのサブURIが含まれます。', 'WebSocketプロキシをコンテキストと組み合わせて使用すると、同じページ上で異なる種類のトラフィックを異なる方法で提供できるため、パフォーマンスを最適化できます。WebSocketトラフィックをWebSocketバックエンドへ送信しつつ、静的コンテキストを設定してLSWSにページの静的コンテンツを提供させたり、LSAPIコンテキストを設定してLSWSにPHPコンテンツを提供させたりできます(どちらもWebSocketバックエンドよりLSWSの方が効率的に処理できます)。');

$_tipsdb['EDTP:GroupDBLocation'] = array('データベースは$SERVER_ROOT/conf/vhosts/$VH_NAME/ディレクトリの下に保存することをお勧めします。');
$_tipsdb['EDTP:UDBgroup'] = array('ここにグループ情報を追加すると、この情報がリソース認可に使用され、このユーザーに関係するグループDB設定は無視されます。','複数のグループを入力する場合は、カンマで区切ります。スペース文字はグループ名の一部として扱われます。');
$_tipsdb['EDTP:accessControl_allow'] = array('サーバー、バーチャルホスト、コンテキストレベルでアクセス制御を設定できます。 サーバーレベルでアクセス制御がある場合、サーバー規則が満たされた後にバーチャルホスト規則が適用されます。','入力形式は、192.168.0.2のようなIP、192.168.*のようなサブネットワーク、または192.168.128.5/255.255.128.0のようなサブネット/ネットマスクです。','信頼できるIPまたはサブネットワークをお持ちの場合は、192.168.1.* Tのような末尾の &quot;T&quot;を追加して許可リストに指定する必要があります。 信頼できるIPまたはサブネットワークは、接続/スロットリング制限によって制限されません。');
$_tipsdb['EDTP:accessControl_deny'] = array('特定のアドレスからのアクセスを拒否するには、許可リストに &quot;ALL&quot;を入れ、サブネットまたはIPを拒否リストに入れます。 特定のIPまたはサブネットのみがサイトにアクセスできるようにするには、拒否リストに &quot;ALL&quot;を入れ、許可リストにそのアドレスを指定します。');
$_tipsdb['EDTP:accessDenyDir'] = array('特定のディレクトリへのアクセスを拒否するにはフルパスを入力します。 *を続けてパスを入力すると、すべてのサブディレクトリが無効になります。','パスは、絶対パスでも、$SERVER_ROOTパスでも構いません。カンマで区切ります。','<b>シンボリックリンクを実行</b>と<b>シンボリックリンクを確認</b>の両方が有効な場合、シンボリックリンクは拒否されたディレクトリと照合されます。');
$_tipsdb['EDTP:accessLog_fileName'] = array('ログファイルのパスは絶対パスでも、$SERVER_ROOTからの相対パスでもかまいません。');
$_tipsdb['EDTP:aclogUseServer'] = array('必要に応じて、このバーチャルホストのアクセスログを無効にしてディスクI/Oを削減できます。');
$_tipsdb['EDTP:adminEmails'] = array('複数の管理用メールを入力することができます：カンマで区切ります。');
$_tipsdb['EDTP:adminOldPass'] = array('セキュリティ上の理由から、管理者パスワードを忘れた場合は、WebAdminコンソールから変更することはできません。 代わりに以下のシェルコマンドを使用してください： <br><br> /usr/local/lsws/admin/misc/admpass.sh. <br><br> このスクリプトは、入力されたすべての管理ユーザーIDを削除し、1人の管理ユーザーで上書きします。');
$_tipsdb['EDTP:allowBrowse'] = array('静的コンテキストを使用して、URIをドキュメントルートの外部またはその内部のディレクトリにマップできます。 ディレクトリは、ドキュメントルート（デフォルト）、$VH_ROOTまたは$SERVER_ROOTの絶対パスまたは相対パスにすることができます。','「アクセス可能」をチェックすると、このコンテキストで静的ファイルをブラウズできます。 静的ファイルを表示しないように無効にすることができます。 例えば コンテンツを更新するとき。');
$_tipsdb['EDTP:appType'] = array('','');
$_tipsdb['EDTP:as_location'] = array('App Serverコンテキストは、Rack/Rails、WSGI、またはNode.jsアプリケーションを簡単に実行設定するためのものです。&quot;Location&quot;フィールドには、アプリケーションのルート場所を指定するだけで済みます。');
$_tipsdb['EDTP:as_startupfile'] = array('','');
$_tipsdb['EDTP:autoFix503'] = array('<b>503エラーの自動修復</b>を有効にすると、クラッシュが検出された場合に監視プロセスが新しいサーバープロセスを自動的に起動し、 サービスは即座に再開されます。');
$_tipsdb['EDTP:backlog'] = array('ローカルアプリケーションは、Webサーバーによって開始できます。 この場合、パス、バックログ、インスタンス数を指定する必要があります。');
$_tipsdb['EDTP:binPath'] = array('','');
$_tipsdb['EDTP:bubbleWrap'] = array('');
$_tipsdb['EDTP:bubbleWrapCmd'] = array('');
$_tipsdb['EDTP:cgi_path'] = array('CGIコンテキストを使用すると、CGIスクリプトのみを含むディレクトリを指定できます。パスは絶対パス、または$SERVER_ROOT、$VH_ROOT、$DOC_ROOT（デフォルト）からの相対パスにできます。cgi-binディレクトリの場合、パスとURIは&quot;/&quot;で終わる必要があります。');
$_tipsdb['EDTP:checkSymbolLink'] = array('Check-Symbolic-Link制御は、Follow-Symbolic-Linkがオンになっている場合にのみ有効です。 これは、シンボリックリンクがアクセス拒否ディレクトリに対してチェックされるかどうかを制御します。');
$_tipsdb['EDTP:compressibleTypes'] = array('Compressible Typesは、圧縮可能なMIMEタイプをカンマで区切ったリストです。*/*、text/*など、MIMEタイプにはワイルドカード&quot;*&quot;を使用できます。特定のタイプを除外するには、先頭に&quot;!&quot;を付けることができます。&quot;!&quot;を使用する場合は、リストの順序が重要です。たとえば、&quot;text/*, !text/css, !text/js&quot;のようなリストでは、cssとjsを除くすべてのテキストファイルが圧縮されます。');
$_tipsdb['EDTP:ctxType'] = array('<b>静的</b>コンテキストを使用して、URIを文書のルートの外またはその内部のディレクトリにマップできます。','<b>Java Web アプリ</b>コンテキストは、AJPv13準拠のJavaサーブレットエンジンで定義済みのJavaアプリケーションを自動的にインポートするために使用されます。','<b>サーブレット</b>コンテキストは、Webアプリケーションの下で特定のサーブレットをインポートするために使用されます。','<b>Fast CGI</b>コンテキストは、Fast CGIアプリケーションのマウントポイントです。','<b>LiteSpeed SAPI</b>コンテキストを使用して、URIをLSAPIアプリケーションに関連付けることができます。','<b>プロキシー</b>コンテキストでは、このバーチャルホストを透過的な逆プロキシー・サーバーとして外部Webサーバーまたはアプリケーション・サーバーに使用できます。','<b>CGI</b>コンテキストを使用して、ディレクトリにCGIスクリプトのみを指定することができます。','<b>ロードバランサー</b>コンテキストを使用して、そのコンテキストに異なるクラスタを割り当てることができます。','<b>リダイレクト</b>コンテキストでは、内部リダイレクトURIまたは外部リダイレクトURIを設定できます。','<b>Rack/Rails</b>コンテキストは、特にRack/Railsアプリケーションに使用されます。','<b>モジュールハンドラ</b>コンテキストは、ハンドラタイプモジュールのマウントポイントです。');
$_tipsdb['EDTP:docRoot'] = array('ドキュメントルートがまだ存在しない場合、サーバーは自動的には作成しません。ディレクトリが存在し、正しいユーザーが所有していることを確認してください。');
$_tipsdb['EDTP:domainName'] = array('このリスナーに応答させるすべてのドメインを入力します。個々のドメインを区切るには、カンマ&quot;,&quot;を使用します。','指定されていないすべてのドメインを処理するバーチャルホストを1つだけ選択し、ドメインに&quot;*&quot;を入れることができます。');
$_tipsdb['EDTP:enableDynGzipCompress'] = array('動的GZIP圧縮の制御は、GZIP圧縮が有効な場合にのみ有効です。');
$_tipsdb['EDTP:enableExpires'] = array('Expiresは、サーバー/バーチャルホスト/コンテキストレベルで設定できます。 低いレベルの設定は、高いレベルの設定を上書きします。 上書き優先度の観点から： <br><br> コンテキストレベル > バーチャルホストレベル > サーバーレベル <br><br>');
$_tipsdb['EDTP:enableRecaptcha'] = array('サーバーレベルでこの設定が<b>Yes</b>に設定されている場合でも、CAPTCHA保護はバーチャルホストレベルで無効にできます。');
$_tipsdb['EDTP:errURL'] = array('さまざまなエラーコードに対してカスタムエラーページを設定できます。');
$_tipsdb['EDTP:expiresByType'] = array('タイプ別の期限のデフォルト設定を上書きします。 各エントリは、&quot;MIME-type=A|Mseconds&quot;の形式であり、間にスペースはありません。 カンマで区切って複数のエントリを入力できます。');
$_tipsdb['EDTP:expiresDefault'] = array('Expires構文では、 &quot;A | Mseconds&quot;は、基準時間（AまたはM）に指定された時間（秒）を加えた後、ファイルの有効期限が切れます。 「A」はクライアントのアクセス時間を意味し、「M」はファイルが変更された時間を意味する。 このデフォルト設定を異なるMIMEタイプで上書きすることができます.A86400は、クライアントのアクセス時間に基づいて1日後にファイルが期限切れになることを意味します。','1時間= 3600秒、1日= 86400秒、1週間= 604800秒、1ヶ月= 2592000秒、1年= 31536000秒という一般的な数字があります。');
$_tipsdb['EDTP:extAppAddress'] = array('アドレスは、192.168.1.3:7777やlocalhost:7777のようなIPv4ソケットアドレス「IP:PORT」、またはUDS://tmp/lshttpd/myfcgi.sockのようなUnixドメインソケットアドレス「UDS://path」にできます。','UDSはchroot環境でchrootされます。','ローカルアプリケーションの場合、セキュリティとパフォーマンスが向上するため、Unixドメインソケットが優先されます。 IPv4ソケットを使用する必要がある場合は、IP部分をlocalhostまたは127.0.0.1に設定します。したがって、アプリケーションは他のマシンからアクセスできません。');
$_tipsdb['EDTP:extAppName'] = array('覚えやすい名前をつけ、他の場所はその名前でこのアプリを参照します。');
$_tipsdb['EDTP:extWorkers'] = array('ロードバランサワーカーは、事前に定義されている必要があります。','使用可能なExtAppタイプは、fcgi（高速CGIアプリケーション）、LSAPIアプリケーション（LSAPIアプリケーション）、サーブレット（サーブレット/JSPエンジン）、プロキシ（Webサーバー）です。','1つの負荷分散クラスタに異なるタイプの外部アプリケーションを混在させることができます。');
$_tipsdb['EDTP:externalredirect'] = array('リダイレクトURIをここに設定します。 外部リダイレクトの場合は、ステータスコードを指定できます。 内部リダイレクトは「/」で始まり、外部リダイレクトは&quot;/&quot;または&quot;http(s)：//&quot;で始まります。');
$_tipsdb['EDTP:extraHeaders'] = array('追加のレスポンス/リクエストヘッダーを指定します。複数のヘッダーディレクティブを1行に1つ追加できます。');
$_tipsdb['EDTP:fcgiapp'] = array('Fast CGIコンテキストは、Fast CGIアプリケーションのマウントポイントです。 Fast CGIアプリケーションは、サーバーレベルまたはバーチャルホストレベルであらかじめ定義されている必要があります。');
$_tipsdb['EDTP:followSymbolLink'] = array('Follow-Symbolic-Linkが有効になっている場合でも、バーチャルホストレベルでそれを無効にできます。');
$_tipsdb['EDTP:gdb_groupname'] = array('グループ名は英字と数字のみで構成してください。');
$_tipsdb['EDTP:gzipCompressLevel'] = array('GZIP圧縮レベルの範囲は1（最小）から9（最大）です。');
$_tipsdb['EDTP:hardLimit'] = array('ウェブページのコンテンツとトラフィックの負荷に応じて、20〜50の間で設定することをお勧めします。');
$_tipsdb['EDTP:indexUseServer'] = array('インデックスファイルには、デフォルトのサーバーレベル設定を使用するか、独自の設定を使用できます。','サーバーレベル設定に加えて、自分の設定を追加できます。','インデックスファイルを無効にするには、サーバーレベル設定を使用しないようにし、バーチャルホストレベルの設定を空白のままにします。','Contextレベルで「自動インデックス」を有効/無効にできます。');
$_tipsdb['EDTP:javaServletEngine'] = array('サーブレットエンジンが別のマシン上で動作する場合は、webappsディレクトリのコピーをローカルに作成することをお勧めします。 それ以外の場合は、アクセス可能な一般的なネットワークドライブにファイルを配置する必要があり、パフォーマンスに影響する可能性があります。');
$_tipsdb['EDTP:javaWebApp_location'] = array('Java Webアプリケーションコンテキストは、AJPv13準拠のJavaサーブレットエンジンで事前定義されたJavaアプリケーションを自動的にインポートするために使用されます。サーブレットエンジンは、外部アプリケーションセクション（サーバーまたはバーチャルホストレベルのいずれか）に設定する必要があります。','ロケーションはWEB-INF/サブディレクトリを含むWebアプリケーションファイルを含むディレクトリです。','Webサーバーは、自動的に &quot;location&quot;で指定されたドライバーのWEB-INF/web.xmlであるWebアプリケーションの構成ファイルをインポートします。');
$_tipsdb['EDTP:listenerIP'] = array('一覧からIPアドレスを選択します。特定のアドレスを指定しない場合、システムはこのマシン上の利用可能なすべてのIPアドレスにバインドします。');
$_tipsdb['EDTP:listenerName'] = array('リスナーに理解しやすく覚えやすい名前を付けてください。');
$_tipsdb['EDTP:listenerPort'] = array('このリスナーに対して、このIP上に一意のポート番号を入力します。 スーパーユーザー（root）のみが1024より小さいポートを使用できます。 ポート80はデフォルトのHTTPポートです。 ポート443はデフォルトのHTTPSポートです。');
$_tipsdb['EDTP:listenerSecure'] = array('<b>セキュア</b>で「はい」を選択すると、このリスナーはHTTPSを使用します。その後、SSL設定で追加設定を行う必要があります。');
$_tipsdb['EDTP:logUseServer'] = array('<b>サーバーのログを使用する</b>に「はい」を選択すると、ログはサーバーレベルで設定されたサーバーファイルに書き込まれます。');
$_tipsdb['EDTP:log_enableStderrLog'] = array('stderrログはサーバーログと同じディレクトリにあります。有効にすると、外部アプリケーションからstderrへのすべての出力がこのファイルに記録されます。');
$_tipsdb['EDTP:log_fileName'] = array('ログファイルのパスは絶対パス、または$SERVER_ROOTからの相対パスで指定できます。');
$_tipsdb['EDTP:log_rollingSize'] = array('現在のログファイルがローテーションサイズを超えると、新しいログファイルが作成されます。ファイルサイズはバイト単位で、10240、10K、1Mなど複数の入力形式を使用できます。');
$_tipsdb['EDTP:maxCGIInstances'] = array('CGIプログラムが使用できるリソースを制限します。 これは、DoS攻撃に役立ちます。','最大CGIインスタンスは、Webサーバーが起動できるCGIプロセスの数を制御します。');
$_tipsdb['EDTP:maxReqHeaderSize'] = array('数値は10240または10Kのように表せます。');
$_tipsdb['EDTP:mime'] = array('MIME設定は前のページから編集できます。 mime構成ファイルの場所は、絶対パスでも、$SERVER_ROOTからの相対パスでも指定できます。');
$_tipsdb['EDTP:namespace'] = array('');
$_tipsdb['EDTP:namespaceConf'] = array('');
$_tipsdb['EDTP:nodeBin'] = array('');
$_tipsdb['EDTP:phpIniOverride'] = array('');
$_tipsdb['EDTP:procSoftLimit'] = array('プロセスソフト/ハードリミットは、1人のユーザーに許可されるプロセスの数を制御します。 これには、CGIアプリケーションによって生成されたすべてのプロセスが含まれます。 設定されていない場合、OSレベル制限が使用されます。','0または空に設定すると、すべてのソフト/ハードリミットにオペレーティングシステムのデフォルト値が使用されます。','ソフトリミットは、カーネルが対応するリソースに対して実施する値です。 ハードリミットは、ソフトリミットの上限として機能します。');
$_tipsdb['EDTP:proxyWebServer'] = array('プロキシコンテキストは、このバーチャルホストを透過的なリバースプロキシサーバとして機能させ、外部のWebサーバまたはアプリケーションサーバに提供します。','外部Webサーバーは、サーバーまたはバーチャルホストレベルで外部アプリケーションの下にあらかじめ定義されている必要があります。');
$_tipsdb['EDTP:realm'] = array('コンテキストは、バーチャルホストセキュリティセクションで設定されている定義済みのレルムで保護することができます。 必要に応じて、代替名と追加要件を指定することができます。');
$_tipsdb['EDTP:recaptchaAllowedRobotHits'] = array('');
$_tipsdb['EDTP:recaptchaBotWhiteList'] = array('');
$_tipsdb['EDTP:recaptchaMaxTries'] = array('');
$_tipsdb['EDTP:recaptchaRegConnLimit'] = array('');
$_tipsdb['EDTP:recaptchaSecretKey'] = array('');
$_tipsdb['EDTP:recaptchaSiteKey'] = array('サーバーレベルのサイトキー/シークレットキーのペアは、サーバーが複数のドメインを管理する場合、ドメインチェックをスキップするように設定する必要があります。そうしないと、CAPTCHA検証が正しく機能しません。');
$_tipsdb['EDTP:recaptchaSslConnLimit'] = array('');
$_tipsdb['EDTP:recaptchaType'] = array('');
$_tipsdb['EDTP:recaptchaVhReqLimit'] = array('');
$_tipsdb['EDTP:restrained'] = array('共有ホスティング環境ではアクセス制限をオンにします。');
$_tipsdb['EDTP:reusePort'] = array('');
$_tipsdb['EDTP:rewriteMapLocation'] = array('場所のURIを入力します。 URIは「/」で始まる必要があります。');
$_tipsdb['EDTP:rewriteRules'] = array('ここでは、Apacheのバーチャルホスト設定ファイルにあるようなバーチャルホストレベルのRewriteルールのみを使用してください。ドキュメントルートレベルのRewriteルールは追加しないでください。.htaccess由来のドキュメントルートレベルのRewriteルールがある場合は、代わりにURI「/」の静的コンテキストを作成し、そこにRewriteルールを追加してください。');
$_tipsdb['EDTP:rubyBin'] = array('<b>Rubyパス</b>は、Ruby実行ファイルの絶対パスです。 たとえば、/usr/local/bin/ruby。');
$_tipsdb['EDTP:scgiapp'] = array('SCGIコンテキストはSCGIアプリケーションのマウントポイントです。SCGIアプリケーションは、サーバーレベルまたはバーチャルホストレベルで事前に定義されている必要があります。');
$_tipsdb['EDTP:serverName'] = array('サーバープロセスのユーザーおよびグループ設定は変更できません。これはインストール時に設定されています。 このオプションを変更するには再インストールが必要です。');
$_tipsdb['EDTP:servletEngine'] = array('サーブレットエンジンが別のマシン上で動作する場合は、webappsディレクトリのコピーをローカルに作成することをお勧めします。 それ以外の場合は、アクセス可能な一般的なネットワークドライブにファイルを配置する必要があり、パフォーマンスに影響する可能性があります。');
$_tipsdb['EDTP:shHandlerName'] = array('CGIおよびModule Handlerを除き、他のハンドラタイプは「外部アプリ」セクションで事前に定義する必要があります。');
$_tipsdb['EDTP:sndBufSize'] = array('数字は、10240,10K、または1Mとして表すことができます。','送信/受信バッファサイズが0の場合、OSのデフォルトTCPバッファサイズが使用されます。');
$_tipsdb['EDTP:softLimit'] = array('ここでIPレベルのスロットル制限を設定します。 番号は4K単位に切り上げられます。 スロットリングを無効にするには、「0」に設定します。','猶予期間中のハードリミットの間は、接続数が一時的にソフトリミットを超えることがあります。 猶予期間の後、まだソフトリミットを超えている場合、そのIPからの禁止期間の接続は許可されません。');
$_tipsdb['EDTP:sslSessionCache'] = array('セッションキャッシュを使用すると、クライアントはSSLハンドシェイクを再実行せずに、設定された時間内にセッションを再開できます。これは、<b>セッションキャッシュを有効にする</b>を使用してクライアントにセッションIDを割り当てるか、セッションチケットを作成して使用することで実現できます。');
$_tipsdb['EDTP:sslSessionTicketKeyFile'] = array('チケットがサーバーによって生成されている場合、セッションチケットは自動的にローテーションされます。 <b>SSLセッションチケットキーファイル</b>オプションを使用して独自のセッションチケットを作成および管理する場合は、cronジョブを使用してチケットを自分自身でローテーションする必要があります。');
$_tipsdb['EDTP:swappingDir'] = array('スワッピングディレクトリは/tmpなどのローカルディスク上に配置することをお勧めします。 ネットワークドライブは必ず避けてください。設定されたメモリI/Oバッファが使い果たされるとスワップが発生します。');
$_tipsdb['EDTP:userDBLocation'] = array('データベースは$SERVER_ROOT/conf/vhosts/$VH_NAME/ディレクトリの下に保存することをお勧めします。');
$_tipsdb['EDTP:uwsgiapp'] = array('uWSGIコンテキストはuWSGIアプリケーションのマウントポイントです。uWSGIアプリケーションは、サーバーレベルまたはバーチャルホストレベルで事前に定義されている必要があります。');
$_tipsdb['EDTP:vhRoot'] = array('全てのディレクトリがあらかじめ存在する必要があります。 このWebインターフェイスでは、ディレクトリは作成されません。 新しいバーチャルホストを作成する場合は、空のルートディレクトリを作成して最初から設定できます。または、パッケージに同梱されている&quot;Example&quot;バーチャルルートをこのバーチャルホストルートにコピーし、ユーザー所有に変更できます。','バーチャルホストルート（$VH_ROOT）は、絶対パス、または$SERVER_ROOTからの相対パスにできます。');
$_tipsdb['EDTP:vhaccessLog_fileName'] = array('ログファイルのパスは絶対パス、または$SERVER_ROOT、$VH_ROOTからの相対パスにできます。');
$_tipsdb['EDTP:vhadminEmails'] = array('カンマで区切って複数の管理用メールを入力できます。');
$_tipsdb['EDTP:vhlog_fileName'] = array('ログファイルのパスは、絶対パス、または$SERVER_ROOT、$VH_ROOTからの相対パスにできます。 ログレベルをDEBUGに設定する場合は、サーバーログレベルもDEBUGに設定する必要があります。 デバッグレベルはサーバーDEBUGレベルで制御されます。 DEBUGはサーバーパフォーマンスに大きく影響し、ディスク容量をすぐに埋める可能性があるため、必要な場合にのみ使用してください。');
$_tipsdb['EDTP:virtualHostName'] = array('このリスナーにマップするバーチャルホストを選択します。','マッピングするバーチャルホストを設定していない場合は、この手順をスキップして後で戻ってください。');
$_tipsdb['EDTP:wsgiBin'] = array('');
