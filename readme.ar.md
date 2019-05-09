<div dir="rtl">
<img
  alt="لغة الأسس البرمجية"
  align="left"
  src="https://alusus.net/Resources/logo.en.gif"
/>

# لغة الأسس البرمجية [[Engligh]](readme.md)

صُمّمت لغة الأسُس لتكون لغةً لكل ما يتعلّق بالبرمجة وذلك بجعل خواص اللغة قابلة للإضافة بشكل متحرّك (ديناميكي) وعلى مستوى البرنامج، أي بتمكين كل برنامج من اختيار خواص اللغة التي يحتاجها، وهذا يمكّن اللغة من:
* الامتداد عموديا للجمع بين الصفات منخفضة المستوى مثل تلك التي تتمتع بها ++C والصفات مرتفعة المستوى مثل تلك التي تتمتع بها لغات كبايثون وروبي وغيرهما.
* التوسع أفقياً لتغطية كافة مجالات البرمجة ما يغني المستخدم عن الحاجة لاستخدام لغات متعددة.
* جعل توسيع اللغة مفتوحاً للجميع وبشكل لا مركزي ما يمكّن المستخدمين من ابتكار أنماط وتقنيات برمجية جديدة دون الحاجة للبدء من الصفر وابتكار لغة جديدة.

تصور أن تتمكن من كتابة شفرة الخادم والزبون والتعامل مع قواعد البيانات والمظللات الرسومية وغيرها بنفس اللغة. تصور أيضاً أن يكون تبديل أنماط البرمجة ممكناً بتبديل المكتبات المستخدمة بدل اللجوء للغة مختلفة. تصور أن تكتب برنامجك بلغة مرتفعة المستوى مع الاحتفظ بالقدرة على اللجوء إلى مستوى منخفض لكتابة العناصر التي تحتاج سرعة أداء قصوى. الهدف من لغة الأسس جعل ذلك كله ممكنا في نهاية المطاف، لكنها ما زالت في مراحلها المبكرة وهي حالياً تدعم البرمجة الإجرائية المنخفضة المستوى والعمل جار على توسيعها لدعم أنماط أخرى وتمكين المستخدمين من دعم أنماط أخرى بأنفسهم عبر المكتبات.

قم بزيارة [alusus.net](https://alusus.net) للمزيد من المعلومات.

## التنزيل

حاليًا Alusus يدعم Linux فقط. تفضل بزيارة صفحة [التنزيلات](https://alusus.net/download) للاطلاع على حزم deb و rpm المبنية مسبقًا.

## التوثيق

لا يزال هناك نقص في الوثائق، ولكن يمكنك العثور على بعض وثائق التصميم عالية المستوى والأمثلة في صحفة
[التوثيق](https://alusus.net/documentation).

## هيكلية المجلدات

* `/Doc`: يحتوي كل الوثائق الخاصة بالمشروع مثل تصميم اللغة ومعاييرها ووثائق الشفرة المصدرية.

* `/Sources`: يحتوي على الشفرة المصدرية الكاملة.
  - `/Sources/Core`: يحتوي على الشفرة المصدرية للقلب.
  - `/Sources/Spp`: يحتوي على الشفرة المصدرية لمكتبة نمط البرمجة المعياري.
  - `/Sources/Srt`: يحتوي على الشفرة المصدرية لمكتبة التنفيذ المعيارية.
  - `/Sources/Tests`: يحتوي على الاختبارات الآلية للقلب والمكتبات المعيارية.

* `/Notices_L18n`: يحتوي على ترجمات إشعارات البناء.

* `/Examples`: يحتوي على أمثلة مكتوبة بلغة الأسُس.

* `/Tools`: يحتوي على شفرات مختلفة للمساعدة في عملية التطوير.

## ساعدنا

يحتاج فريقنا إلى متطوعين للمساهمة في هذا المشروع. انضم إلينا في جعل المستقبل أفضل للمبرمجين. زيارة
[تطوير](https://alusus.net/dev) صفحة لمزيد من المعلومات.

أو بإمكانك المساعدة من خلال التبرع للمشروع<br/>
[![PayPal - The safer, easier way to pay online!](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://paypal.me/alusus)

## المساهمين

* [sarmadka](https://github.com/sarmadka) - **سرمد خالد عبد الله**<br/>
&lt;sarmad@alusus.org&gt; *المبتكر والمهندس الرئيسي*
* [falhumai96](https://github.com/falhumai96) - **فيصل الحميميدي**<br/>
&lt;falhumai96@gmail.com&gt; *مطور*
* [xlmnxp](https://github.com/xlmnxp) - **سالم يسلم صالح**<br/>
&lt;s@sy.sa&gt; *مطور*
* [rafidka](https://github.com/rafidka) - **رافد الحميميدي**<br/>
&lt;rafidka@gmail.com&gt; *مطور*
* **هشام والي علمي** *مطور*

## حقوق النشر

جميع الحقوق محفوظة 2019م\1440هـ.
حقوق هذا العمل مملوكة من قبل سرمد خالد عبد الله.

## [الرخصة](license.txt)

نُشرت معايير لغة الأسُس ووثائقها وشفرتها المصدرية والتنفيذية وأمثلتها برخصة
الأسُس العامة (Alusus Public License)، الإصدار 1.0، والمضمّنة في هذا المجلد
والمتوفرة أيضاً على الرابط [https://alusus.net/alusus_license_1_0](https://alusus.net/alusus_license_1_0).
يرجى قراءة الرخصة قبل استخدام أو نسخ أي من هذه الملفات. استخدامك لأي من الملفات في هذا
المجلد إقرار منك أنك قرأت هذه الرخصة ووافقت على جميع فقراتها.

رخصة الأسُس العامة صُممت لجعلها لغة مفتوحة المصدر وفي نفس الوقت للمحافظة على
معايير اللغة من التشتت مبكراً إلى لغات متعددة غير متوافقة مع بعضها. تتيح
الرخصة ما يلي:
* الحصول على اللغة مجاناً بصيغتيها المصدرية والرقمية واستخدامها لكتابة البرامج
  أياً كانت طبيعتها سواء كانت تجارية أو غير ذلك.
* تعديل اللغة واستخدام النسخة المعدلة لكتابة البرامج أياً كانت طبيعتها سواء كانت
  تجارية أو غير ذلك.
* إعادة نشر النسخة الأصلية غير المعدلة سواء بصيغتها الرقمية أو المصدرية. تمنع
  الرخصة نشر نسخ معدلة إلا بموافقة مسبقة من Alusus Software Ltd.
</div>