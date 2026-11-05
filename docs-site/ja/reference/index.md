---
layout: default
lang: ja
title: 操作リファレンス
permalink: /ja/reference/
section: reference
alternate: /reference/
---

<p class="eyebrow">numeric 0.2.0 reference</p>

# 操作リファレンス

各操作の利用方法、入力の順番、結果、条件を確認できます。操作名を選ぶと該当箇所へ移動します。[その他のインターフェース](#other-interfaces)には専用レコードで使用できない機能もまとめています。

<h2 id="reading-fields">フィールドの読み方</h2>

| フィールド | 役割 |
| --- | --- |
| `OP` | 実行する操作名。データベース読み込み時に設定。 |
| `INP` | 入力が一つの操作で使う入力PVリンク。 |
| `INPA`〜`INPP` | 順番付きの入力PVリンク。以下のA、B、C、Dはこのリンクを表します。 |
| `ARGA`〜`ARGP` | 入力PVとは別の、文字列形式の**設定引数**。引数がない操作では空欄にします。 |
| `NMAX`、`OMAX` | 入力・出力の最大要素数。`OMAX`は`numericArray`用。 |
| `VAL`、`NORD`、`NIN` | 結果と要素数。`numericScalar.NORD`は最初の入力数、`numericArray.NORD`は出力数、`NIN`は最初の入力数。 |
| `PROC`、`STAT`、`SEVR` | 処理の実行、警報の種類と重大度。 |

一入力では`INP`か`INPA`のどちらかを使い、併用しません。複数リンクはAから連続して設定します。リンクは順番に読み取られ、同時刻のスナップショットにはなりません。定数リンクは使えません。入力や引数を変更しただけでは処理されないため、必要なタイミングで処理してください。ARGは空白なし・38文字以内で指定します。空でないARGは対応する従来フィールドを優先し、空欄に戻すと従来値か既定値に戻ります。未使用ARGに値を設定するとINVALIDになります。[レコードの詳細](https://github.com/akared819/numeric/blob/main/docs/numeric-array.md)。

失敗時は古い`VAL`が残ることがあるため、値を使う前に`SEVR`を確認します。配列の容量と有効要素数は異なります。入力型はDOUBLE、FLOAT、LONG、正確に変換できるローカルINT64です。[変換の詳細](https://github.com/akared819/numeric/blob/main/docs/numeric-scalar.md)。

### 例：相関係数

```db
record(numericScalar, "SIGNAL:CORR") {
    field(OP,   "correlation")
    field(INPA, "SIGNAL:X NPP NMS")
    field(INPB, "SIGNAL:Y NPP NMS")
    field(NMAX, "4096")
}
```

両方の波形を更新してから`SIGNAL:CORR.PROC`を処理し、`VAL`と`SEVR`を確認します。PV名は実際の名前に置き換えてください。実際に動く例は[multi-input.db](https://github.com/akared819/numeric/blob/main/examples/multi-input.db)です。

<h2 id="operations">操作一覧</h2>

{% for op in site.data.operations %}[`{{ op.name }}`](#{{ op.name }}){% unless forloop.last %} · {% endunless %}{% endfor %}

{% for op in site.data.operations %}
{% assign detail = site.data.operation_details[op.name] %}
### `{{ op.name }}`

**利用方法：** {% if op.interface == 'scalar' %}`numericScalar`{% elsif op.interface == 'array' %}`numericArray`{% else %}aSub／ネイティブレジストリ。専用レコードでは使用不可{% endif %}。

**入力：** {{ op.inputs_ja }}。

{% for input in detail.types %}- `{{ input.name }}`：{% if input.shape == 'S' %}有限の数値スカラー1個{% else %}数値波形{% endif %}{% if op.interface == 'asub' %}（aSub入力、DOUBLE）{% else %}（DOUBLEに変換されるPVリンク。必須、既定値なし）{% endif %}。
{% endfor %}

**戻り値：** {% if detail.output_shape == 'S' %}`VAL`にDOUBLEスカラー1個{% elsif detail.output_shape == 'A' %}`VAL`にDOUBLE配列{% else %}以下に示す順番のDOUBLEスカラー{{ detail.output_shape.size }}個{% endif %}。{{ op.result_ja }}

{% if op.args_ja %}**設定引数：** {{ op.args_ja }}{% else %}**設定引数：** なし。{% endif %}

**エラー条件：** 例えば{{ detail.error_ja }}の場合です。無効なバッファー、未対応の非有限入力、GSLの未初期化、数値計算・メモリー確保の失敗などでもエラーになります。専用レコードでは`CALC/INVALID`となり、前回の`VAL`が残ることがあります。`SEVR`を確認してください。詳細な条件はリンク先を参照してください。

**使用例：** {% if op.interface == 'asub' %}新しいインスタンスのaSub入力として{% else %}次の入力PVと設定引数を使用します{% endif %}。

{% for value in detail.example_inputs %}- `{{ value.name }}` = `{{ value.value }}`
{% endfor %}
結果：`{{ detail.example_output }}`{% if detail.output_shape.size > 1 %}（出力の順番）{% endif %}。浮動小数点の丸めがある結果は近似値です。{% if op.name == 'running_stats' %}初回サンプルでは標本分散と標本標準偏差が未定義のため`NaN`です。{% endif %}

{% if op.interface != 'asub' %}例えば、下記のソースPVを別途用意してから、次の結果レコードを設定します。この例自体はソースPVや上記の値を作成しません。

```db
record(numeric{% if op.interface == 'scalar' %}Scalar{% else %}Array{% endif %}, "EXAMPLE:RESULT") {
    field(OP, "{{ op.name }}")
{% for input in detail.types %}    field({{ input.name }}, "EXAMPLE:{{ input.name }} NPP NMS")
{% endfor %}{% for value in detail.example_inputs %}{% if value.name contains 'ARG' %}    field({{ value.name }}, "{{ value.value }}")
{% endif %}{% endfor %}    field(NMAX, "128")
{% if op.interface == 'array' %}    field(OMAX, "128")
{% endif %}}
```

各ソースPVに上の値を設定し、`EXAMPLE:RESULT.PROC`を処理してから`VAL`と`SEVR`を読みます。配列結果は`caget`の配列形式で読みます。実際のデータに合わせて容量を変更してください。{% endif %}

**詳しい仕様：** [{{ op.doc }}](https://github.com/akared819/numeric/blob/main/docs/{{ op.doc }})（英語）。
{% endfor %}

<h2 id="other-interfaces">その他のインターフェース</h2>

- **aSub：** [バッチ処理と複数出力](https://github.com/akared819/numeric/blob/main/docs/asub-mean.md)、[FFT](https://github.com/akared819/numeric/blob/main/docs/fft.md)、[フィッティング](https://github.com/akared819/numeric/blob/main/docs/fitting.md)、[確認応答付きストリーミング](https://github.com/akared819/numeric/blob/main/docs/streaming-transport.md)。aSubのフィールドと初期化は専用レコードと異なります。
- **ネイティブコールバック：** [`differentiateCentral`、`integrateAdaptive`、`findRootBrent`、`minimizeBrent`、`solveOdeRkf45`](https://github.com/akared819/numeric/blob/main/docs/numerical-differentiation.md)、[`fitNonlinear`](https://github.com/akared819/numeric/blob/main/docs/nonlinear-fitting.md)。C++関数でありレコード操作ではありません。コールバックとコンテキストは同期呼び出しが終わるまで有効にしてください。
- **乱数：** [`UniformRandomStream`](https://github.com/akared819/numeric/blob/main/docs/random-sampling.md)はインスタンスごとにシードを持つネイティブAPIです。レコード操作ではありません。
- **ネイティブレジストリと配列ビュー：** [構成と所有権](https://github.com/akared819/numeric/blob/main/docs/architecture.md)、[型付きデータモデル](https://github.com/akared819/numeric/blob/main/docs/phase10-data-model-plan.md)、公開ヘッダー[`numeric.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numeric.h)、[`numericData.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numericData.h)、[`numericTypedMean.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numericTypedMean.h)、[`numericAck.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numericAck.h)。

利用可能な機能はGSLの一部です。[対応状況一覧](https://github.com/akared819/numeric/blob/main/docs/gsl-coverage.md)を参照してください。

### ネイティブ実行例の試し方

numericをビルドした後、リポジトリのルートから実行します。Baseのパスは自分の環境に変更し、必要ならGSLの共有ライブラリーへのパスを設定してください。

```sh
./tests/native-reference-examples.sh "$(/absolute/path/to/base/startup/EpicsHostArch)"
```

## ネイティブC++ API早見表

以下は同期呼び出しです。特に指定がなければ、入力と出力の所有者は呼び出し側です。`Result`の`Status`とメッセージを確認してから出力を使います。GSLのプロセス全体の設定は、単一スレッドの起動時に`initializeGsl()`で初期化します。`isGslInitialized()`はnumeric側の初期化状態を返します。IOCコマンドは`numericInitialize`です。

| API | 入力と結果 |
| --- | --- |
| `Registry::add`、`addWithoutArguments` | メタデータと生成関数を登録し、成功したかを返します。 |
| `Registry::find`、`create`、`registerBuiltins` | 名前からメタデータを検索、独立した操作を生成、または組み込み操作を登録します。検索・生成のnullを確認してください。 |
| `Operation::execute`、`reset` | 入出力配列とそれぞれの個数を指定して実行、または状態と資源をリセットします。バッファーの所有者は呼び出し側です。 |
| `validateArgumentSchema`、`validateArguments` | 引数定義、または順番付きの型付き引数を検証します。引数間の追加検証も指定できます。 |
| `resolveArguments`、`parseArgument`、`setLegacyArguments` | 指定値と末尾の既定値を呼び出し側の領域に展開、一つの厳密なASCII値を解析、または従来のスカラー入力欄へ引数を対応付けます。容量と失敗時の規則は[`numeric.h`](https://github.com/akared819/numeric/blob/main/numericApp/src/numeric.h)を参照。 |
| `validateArray`、`validateWritableArray` | 型付き配列ビューの形状、ストライド、バイト容量を検証します。rank 0はスカラー、最大rankは8です。 |
| `readElement`、`writeElement`、`convertElement` | 指定要素の読み書き、または正確な型変換を行います。書き込みは同じ型のみ、複素数から実数への変換は虚部が厳密に0の場合のみです。 |
| `TypedMean`、`TypedComplexMean` | 最大要素数を指定して生成し、`reserve`で作業領域を準備、`execute`で`ArrayView`から平均を計算、`reset`で領域を解除します。同一インスタンスの並行利用は同期が必要です。 |
| `AcknowledgedStream::create`、`submit` | ストリームを生成し、セッション番号・連番付きのサンプル／リセットを送ります。確認応答と統計を受け取ります。アクセスを直列化し、[単一プロデューサーの規約](https://github.com/akared819/numeric/blob/main/docs/streaming-transport.md)に従ってください。 |
| `UniformRandomStream::initialize`、`fill`、`reset` | 32ビットのシードで初期化、呼び出し側の配列に生成、または同じシードから再開します。インスタンスは独立しており、同時アクセスは安全ではありません。 |
| `differentiateCentral` | 関数とコンテキスト、評価点、正の刻みを受け取り、微分値と絶対誤差の推定値を返します。 |
| `integrateAdaptive` | 関数、有限の増加区間、許容誤差、分割上限から、積分値と絶対誤差の推定値を返します。 |
| `findRootBrent` | 関数、根を挟む区間、許容誤差、反復上限から根と区間誤差を返します。 |
| `minimizeBrent` | 関数、下限・初期値・上限、許容誤差、反復上限から最小位置と区間誤差を返します。 |
| `solveOdeRkf45` | 微分方程式のコールバック、時間区間、初期状態、次元、刻み、許容誤差、上限を受け取り、最終状態を呼び出し側の配列に書き込みます。 |
| `fitNonlinear` | 残差コールバック、初期パラメーター、残差数、許容誤差、反復上限を受け取り、推定パラメーターと残差平方和を返します。 |

失敗時の出力保持、コールバックの制約、数値上の上限と実例は[コールバック](https://github.com/akared819/numeric/blob/main/docs/numerical-differentiation.md)、[フィッティング](https://github.com/akared819/numeric/blob/main/docs/nonlinear-fitting.md)、[乱数](https://github.com/akared819/numeric/blob/main/docs/random-sampling.md)の各ガイドを参照してください。GSLの誤差推定は保証された誤差上限ではありません。

<h2 id="native-numerical">ネイティブ数値APIの詳細</h2>

以下はC++ APIです。引数は明記されたものを除き既定値がなく、すべて指定します。`Result.status == Status::Ok`を確認してから出力を使ってください。コールバックとコンテキストは呼び出し中有効に保ち、GSLエラー処理を初期化します。推定誤差は厳密な上限ではありません。

[完全な実行例（コールバック定義を含む）](https://github.com/akared819/numeric/blob/main/examples/native-api-reference.cpp)では、`square(x)=x*x`、`rootFn(x)=x*x-2`、`minimumFn(x)=(x-2)^2`、`ode`は`dx/dt=x`、`residual`は3点の指数モデル残差です。以下の呼び出しはこの例の変数を使います。

{% for api in site.data.native_details %}[`{{ api.name }}`](#{{ api.name | downcase | remove: ":" }}){% unless forloop.last %} · {% endunless %}{% endfor %}

{% for api in site.data.native_details %}
### `{{ api.name }}`

```cpp
{{ api.signature }}
```

**引数：** {{ api.parameters_ja }}

**戻り値：** `Result`（状態とメッセージ）。成功時：{{ api.returns_ja }}

**エラー条件：** {{ api.errors_ja }}

**使用例：**

```cpp
auto result = {{ api.example }};
// result.status == Status::Ok: {{ api.expected }}
```
{% endfor %}

<h2 id="native-core">ネイティブコア・ユーティリティAPIの詳細</h2>

以下はC++ APIであり、専用レコードのフィールドではありません。型・引数・結果・エラーを示します。宣言に明記された既定値以外の引数は必須です。各スニペットは[完全な実行例](https://github.com/akared819/numeric/blob/main/examples/native-core-reference.cpp)からの抜粋です。実際のコードでは全戻り値とポインターを確認し、バッファーを呼び出し中有効に保ってください。

### 共通の型と所有権

| 型 | フィールドと意味 |
| --- | --- |
| `Input` | 呼び出し側所有の`const double* data`、有効要素`count`、アクセス可能な`capacity`、要素単位`stride`、`Type`と`Shape`。入力の容量と有効数は別です。 |
| `Output` | 呼び出し側所有の`double* data`、`capacity`、操作が設定する`count`、`Type`と`Shape`。 |
| `Descriptor` / `Type` / `Shape` | 入出力の型と形状。レジストリ操作は現在`Float64`と`Scalar`／`Array`を使います。 |
| `Result` / `Status` | 状態は`Ok`、`InvalidArgument`、`NumericalError`、`NotInitialized`。`message`は診断用文字列です。 |
| `ArgumentValue` / `ArgumentDescriptor` | 型付きの整数・浮動小数・真偽・列挙値と、名前、必須、既定値、境界、選択肢、従来入力への対応付け。 |
| `Metadata` | 名前、分類、入出力記述子、引数定義、状態の有無、所有権とエラーの説明。`Registry::find`の戻り値はレジストリが所有します。 |
| `ArrayView` / `MutableArrayView` | 呼び出し側所有の領域、容量（バイト）、`ElementType`、rank、各次元の要素数とバイトストライド。配列データ、形状、ストライドは使用中有効にしてください。 |
| `ElementValue` | `ElementType`と整数または実部・虚部を保持。複素数は実部・虚部の順です。 |
| `Event` / `Acknowledgment` | `Event`はsession、sequence、種別、値。確認応答はイベント、7個の統計、再送を示す`replayed`を保持。 |
| `ScalarCallback` | `double (*)(double x, void* context)`。有限の関数値を返す同期関数。 |
| `OdeCallback` | `bool (*)(double time, const double* state, double* derivative, std::size_t dimension, void* context)`。全導関数を書いてtrueを返す。 |
| `ResidualCallback` | `bool (*)(const double* parameters, std::size_t parameterCount, double* residuals, std::size_t residualCount, void* context)`。全残差を書いてtrueを返す。 |
| `Factory` / `ArgumentValidator` | それぞれ`std::unique_ptr<Operation> (*)()`、`Result (*)(const ArgumentValue*, std::size_t)`。 |

{% for api in site.data.native_core_details %}[`{{ api.name }}`](#{{ api.name | downcase | remove: ":" }}){% unless forloop.last %} · {% endunless %}{% endfor %}

{% for api in site.data.native_core_details %}
### `{{ api.name }}`

```cpp
{{ api.signature }}
```

**引数と型：** {{ api.parameters_ja }}

**戻り値：** {{ api.returns_ja }}

**エラー条件：** {{ api.errors_ja }}

**使用例：**

```cpp
{{ api.example }}
```
{% endfor %}


<h2 id="asub-reference">aSub関数の詳細</h2>

各関数は`aSub`レコードの`SNAM`に名前を設定します。A、B、CはaSubの入力で、対応する`INPA`などから取得されます。**専用レコードの`ARGA`とは別物です。** 使用する入力と出力はDOUBLEに設定します。波形入力は、最大入力長より1要素多い`NO*`を確保し、出力には必要な`NOV*`を確保してください。これは切り詰めを検出するためです。例外を含む詳しい制約は各リンク先を確認してください。`numericInitialize`は`iocInit`前に実行します。

数値の設定に自動的な既定値はありません。リンクや定数をaSubに明示して設定してください。値を更新しただけでは通常は処理されません。入力取得が関数呼び出し前に失敗した場合は、numeric側の警報処理を通らないことがあります。失敗時は古い出力が残ることがあるため、警報と入力の鮮度を確認してください。確認応答付きストリーミングは別の処理規約を持ちます。

{% for api in site.data.asub_details %}[`{{ api.name }}`](#{{ api.name | downcase | remove: ":" }}){% unless forloop.last %} · {% endunless %}{% endfor %}

{% for api in site.data.asub_details %}
{% assign operation = site.data.operation_details[api.operation] %}
### `{{ api.name }}`

**設定：** `SNAM="{{ api.name }}"`。{{ api.inputs_ja }}。使用する入力と出力はDOUBLE。

**引数の型・既定値：** 上記のA〜CはDOUBLE入力で、`ARGA`ではありません。波形は`NO*`の容量と`NE*`の有効数を区別し、スカラーは1要素です。操作固有の数値に自動の既定値はありません。

{% if api.note_ja %}**補足条件：** {{ api.note_ja }}

{% endif %}**戻り値：** {{ api.outputs_ja }}。成功時の出力個数は対応する`NEV*`を参照します。

**エラー条件：** {{ operation.error_ja }}など。無効な型・容量、数値エラー、GSL未初期化も失敗の原因です。{% if api.name == 'numericRunningMean' or api.name == 'numericRunningStats' %}同じ連番の再処理は更新せず、`VAL=1`で無視されます。{% elsif api.name == 'numericAcknowledgedStats' %}確認応答プロトコルの連番やセッションも検証します。{% else %}関数内で検出した失敗は通常`CALC/INVALID`となり、出力リンクを更新しません。{% endif %}

**使用例：** {% for value in api.example_inputs %}`{{ value.name }}={{ value.value }}`{% unless forloop.last %}、{% endunless %}{% endfor %} → `{{ api.example_output }}`。丸めがある結果は近似値です。{% if api.name == 'numericAcknowledgedStats' %}このイベントは`numericAckSend demo:event demo:acknowledgedStats 1 0 sample 2`で送信でき、別途`.PROC`を書き込まないでください。{% endif %}

[設定例：{{ api.db }}](https://github.com/akared819/numeric/blob/main/examples/{{ api.db }}){% if api.adapt_example %}。このファイルの`SNAM`を`{{ api.name }}`に変更して使用します{% endif %}。{% if api.name == 'numericRealFftForward' %}`FTVB=DOUBLE`、`NOVB >= 2N`が必要で、OUTBは未設定のままにします。{% endif %} [対応する数値操作の詳細](#{{ api.operation }})。
{% endfor %}
