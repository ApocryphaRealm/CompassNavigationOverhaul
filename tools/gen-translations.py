# -*- coding: utf-8 -*-
"""gen-translations.py - builds the eleven CompassNavigationOverhaul_<language>.txt files.

The English key list is extracted from the PATCHED source/UI.cpp by regex on
strings::TR("KEY", "text") so it can never drift from the code. The other ten languages are
this project's own translations of that list, held below as parallel dictionaries.

Writes REPO/dist/Interface/Translations/CompassNavigationOverhaul_<language>.txt for english +
the owner's ten languages (UTF-16LE with a BOM, one "$key<TAB>text" per line, literal "\\n" for
an embedded line break, CRLF records - the SKSE/SkyUI shape AMF's own Strings.cpp reads).

Run: `python tools/gen-translations.py` from the repo root or anywhere (paths are relative to
this script's grandparent directory).
"""
import io
import os
import re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LANGS = ["english", "japanese", "korean", "chinese", "russian", "german", "french", "spanish", "italian", "polish", "czech"]

TR_RE = re.compile(r'strings::TR\(\s*"((?:[^"\\]|\\.)+)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)')


def unescape(s):
    return s.encode("latin-1", "backslashreplace").decode("unicode_escape") if "\\" in s else s


KEY_ARRAY_RE = re.compile(r'constexpr const char\* kLogLevelKeys\[\]\s*=\s*\{([^}]*)\};', re.S)
NAME_ARRAY_RE = re.compile(r'constexpr const char\* kLogLevelNames\[\]\s*=\s*\{([^}]*)\};', re.S)
STR_LIT_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def read_keys():
    path = os.path.join(REPO, "source", "UI.cpp")
    src = io.open(path, "r", encoding="utf-8").read()
    keys = {}
    order = []
    for m in TR_RE.finditer(src):
        key, text = unescape(m.group(1)), unescape(m.group(2))
        if key in keys and keys[key] != text:
            raise RuntimeError(f"duplicate key {key!r} with two different English texts: {keys[key]!r} vs {text!r}")
        if key not in keys:
            order.append(key)
        keys[key] = text

    # The log-level Combo's option labels are looked up by a parallel key array
    # (kLogLevelKeys[i] -> kLogLevelNames[i]) rather than a literal strings::TR(...) call, since
    # the option text is rebuilt into a std::vector per frame. Pair the two arrays positionally.
    km = KEY_ARRAY_RE.search(src)
    nm = NAME_ARRAY_RE.search(src)
    if not km or not nm:
        raise RuntimeError("could not find kLogLevelKeys/kLogLevelNames arrays in source/UI.cpp")
    array_keys = [unescape(s) for s in STR_LIT_RE.findall(km.group(1))]
    array_names = [unescape(s) for s in STR_LIT_RE.findall(nm.group(1))]
    if len(array_keys) != len(array_names):
        raise RuntimeError(f"kLogLevelKeys ({len(array_keys)}) and kLogLevelNames ({len(array_names)}) length mismatch")
    for key, text in zip(array_keys, array_names):
        if key in keys and keys[key] != text:
            raise RuntimeError(f"duplicate key {key!r} with two different English texts: {keys[key]!r} vs {text!r}")
        if key not in keys:
            order.append(key)
        keys[key] = text

    return keys, order


# ------------------------------------------------------------------------------------------------
# Translations for every key found in source/UI.cpp. Plain, literal renderings of the UI text;
# every printf specifier is kept exactly; product names (Skyrim, Apocrypha Menu Framework, Compass
# Navigation Overhaul, Untarnished UI, Dragon's Eye Minimap, Infinity UI) stay untranslated;
# Skyrim's own terms (compass, quest marker, objective, location, distance, undiscovered) use
# each language's in-game vocabulary.
# ------------------------------------------------------------------------------------------------
TRANSLATIONS = {
    "CNO_HelpMark": {
        "japanese": "(?)", "korean": "(?)", "chinese": "(?)", "russian": "(?)", "german": "(?)",
        "french": "(?)", "spanish": "(?)", "italian": "(?)", "polish": "(?)", "czech": "(?)",
    },
    "CNO_SliderNudge": {
        "japanese": "<-->", "korean": "<-->", "chinese": "<-->", "russian": "<-->", "german": "<-->",
        "french": "<-->", "spanish": "<-->", "italian": "<-->", "polish": "<-->", "czech": "<-->",
    },
    "CNO_Title": {
        "japanese": "コンパスとマーカー", "korean": "나침반과 마커", "chinese": "罗盘与标记", "russian": "Компас и маркеры",
        "german": "Kompass und Markierungen", "french": "Boussole et marqueurs", "spanish": "Brújula y marcadores",
        "italian": "Bussola e indicatori", "polish": "Kompas i znaczniki", "czech": "Kompas a značky",
    },
    "CNO_UseMetricUnits": {
        "japanese": "メートル単位を使用", "korean": "미터법 단위 사용", "chinese": "使用公制单位", "russian": "Использовать метрические единицы",
        "german": "Metrische Einheiten verwenden", "french": "Utiliser les unités métriques", "spanish": "Usar unidades métricas",
        "italian": "Usa unità metriche", "polish": "Użyj jednostek metrycznych", "czech": "Použít metrické jednotky",
    },
    "CNO_HelpUseMetricUnits": {
        "japanese": "バニラのフィートの代わりにメートルでマーカーまでの距離を表示します。",
        "korean": "바닐라의 피트 대신 미터 단위로 마커까지의 거리를 표시합니다.",
        "chinese": "以米为单位显示到标记的距离,而非原版的英尺。",
        "russian": "Показывает расстояние до маркеров в метрах вместо стандартных футов.",
        "german": "Zeigt Entfernungen zu Markierungen in Metern statt der Standard-Fuß an.",
        "french": "Affiche les distances aux marqueurs en mètres au lieu des pieds d'origine.",
        "spanish": "Muestra las distancias a los marcadores en metros en lugar de los pies originales.",
        "italian": "Mostra le distanze dagli indicatori in metri invece dei piedi originali.",
        "polish": "Pokazuje odległości do znaczników w metrach zamiast domyślnych stóp.",
        "czech": "Zobrazuje vzdálenosti ke značkám v metrech místo výchozích stop.",
    },
    "CNO_ShowUndiscoveredLocationMarkers": {
        "japanese": "未発見のロケーションマーカーを表示", "korean": "미발견 위치 마커 표시", "chinese": "显示未发现的地点标记",
        "russian": "Показывать маркеры неизвестных мест", "german": "Unentdeckte Ortsmarkierungen anzeigen",
        "french": "Afficher les marqueurs de lieux non découverts", "spanish": "Mostrar marcadores de ubicaciones no descubiertas",
        "italian": "Mostra indicatori dei luoghi non scoperti", "polish": "Pokaż znaczniki nieodkrytych lokacji",
        "czech": "Zobrazit značky neobjevených lokací",
    },
    "CNO_UndiscoveredMeansUnknownMarkers": {
        "japanese": "未発見時は不明のマーカーにする", "korean": "미발견 시 알 수 없는 마커로 표시", "chinese": "未发现时显示为未知标记",
        "russian": "Неизвестное = неизвестный маркер", "german": "Unentdeckt bedeutet unbekannte Markierung",
        "french": "Non découvert = marqueur inconnu", "spanish": "No descubierto = marcador desconocido",
        "italian": "Non scoperto = indicatore sconosciuto", "polish": "Nieodkryte = nieznany znacznik",
        "czech": "Neobjevené = neznámá značka",
    },
    "CNO_HelpUndiscoveredMeansUnknownMarkers": {
        "japanese": "発見するまで、その場所の実際のアイコンの代わりに汎用マーカーを表示します。",
        "korean": "발견하기 전까지 해당 장소의 실제 아이콘 대신 일반 마커를 표시합니다.",
        "chinese": "在你发现该地点之前,显示通用标记而非其真实图标。",
        "russian": "Показывает обычный маркер вместо настоящей иконки места, пока оно не будет обнаружено.",
        "german": "Zeigt eine generische Markierung anstelle des tatsächlichen Symbols des Ortes an, bis du ihn entdeckst.",
        "french": "Affiche un marqueur générique à la place de l'icône réelle du lieu jusqu'à ce que vous le découvriez.",
        "spanish": "Muestra un marcador genérico en lugar del icono real del lugar hasta que lo descubras.",
        "italian": "Mostra un indicatore generico al posto dell'icona reale del luogo finché non lo scopri.",
        "polish": "Pokazuje ogólny znacznik zamiast prawdziwej ikony lokacji, dopóki jej nie odkryjesz.",
        "czech": "Zobrazuje obecnou značku místo skutečné ikony místa, dokud ho neobjevíte.",
    },
    "CNO_UndiscoveredMeansUnknownInfo": {
        "japanese": "未発見時は情報も不明にする", "korean": "미발견 시 정보도 알 수 없음으로 표시", "chinese": "未发现时隐藏信息",
        "russian": "Неизвестное = скрыть информацию", "german": "Unentdeckt bedeutet unbekannte Informationen",
        "french": "Non découvert = informations inconnues", "spanish": "No descubierto = información desconocida",
        "italian": "Non scoperto = informazioni sconosciute", "polish": "Nieodkryte = nieznane informacje",
        "czech": "Neobjevené = neznámé informace",
    },
    "CNO_HelpUndiscoveredMeansUnknownInfo": {
        "japanese": "発見するまで、コンパスにその場所の名前と距離を表示しません。",
        "korean": "발견하기 전까지 나침반에 해당 장소의 이름과 거리를 표시하지 않습니다.",
        "chinese": "在你发现该地点之前,隐藏罗盘上该地点的名称与距离。",
        "russian": "Скрывает название и расстояние до места на компасе, пока оно не будет обнаружено.",
        "german": "Verbirgt den Namen und die Entfernung des Ortes auf dem Kompass, bis du ihn entdeckst.",
        "french": "Masque le nom et la distance du lieu sur la boussole jusqu'à ce que vous le découvriez.",
        "spanish": "Oculta el nombre y la distancia del lugar en la brújula hasta que lo descubras.",
        "italian": "Nasconde il nome e la distanza del luogo sulla bussola finché non lo scopri.",
        "polish": "Ukrywa nazwę i odległość lokacji na kompasie, dopóki jej nie odkryjesz.",
        "czech": "Skryje název a vzdálenost místa na kompasu, dokud ho neobjevíte.",
    },
    "CNO_ShowEnemyMarkers": {
        "japanese": "敵マーカーを表示", "korean": "적 마커 표시", "chinese": "显示敌人标记", "russian": "Показывать маркеры врагов",
        "german": "Feindmarkierungen anzeigen", "french": "Afficher les marqueurs d'ennemis", "spanish": "Mostrar marcadores de enemigos",
        "italian": "Mostra indicatori nemici", "polish": "Pokaż znaczniki wrogów", "czech": "Zobrazit značky nepřátel",
    },
    "CNO_ShowEnemyNameUnderMarker": {
        "japanese": "マーカーの下に敵の名前を表示", "korean": "마커 아래에 적 이름 표시", "chinese": "在标记下方显示敌人名称",
        "russian": "Показывать имя врага под маркером", "german": "Feindnamen unter der Markierung anzeigen",
        "french": "Afficher le nom de l'ennemi sous le marqueur", "spanish": "Mostrar el nombre del enemigo bajo el marcador",
        "italian": "Mostra il nome del nemico sotto l'indicatore", "polish": "Pokaż nazwę wroga pod znacznikiem",
        "czech": "Zobrazit jméno nepřítele pod značkou",
    },
    "CNO_ShowInteriorMarkers": {
        "japanese": "屋内マーカーを表示", "korean": "실내 마커 표시", "chinese": "显示室内标记", "russian": "Показывать маркеры в помещениях",
        "german": "Innenraum-Markierungen anzeigen", "french": "Afficher les marqueurs d'intérieur", "spanish": "Mostrar marcadores de interiores",
        "italian": "Mostra indicatori degli interni", "polish": "Pokaż znaczniki wnętrz", "czech": "Zobrazit značky interiérů",
    },
    "CNO_ShowObjectiveAsTarget": {
        "japanese": "目標をターゲットとして表示", "korean": "목표를 대상으로 표시", "chinese": "将目标显示为焦点",
        "russian": "Показывать цель как активную", "german": "Ziel als aktives Ziel anzeigen",
        "french": "Afficher l'objectif comme cible", "spanish": "Mostrar el objetivo como objetivo activo",
        "italian": "Mostra l'obiettivo come bersaglio", "polish": "Pokaż cel jako aktywny cel",
        "czech": "Zobrazit cíl jako aktivní cíl",
    },
    "CNO_ShowOtherObjectivesCount": {
        "japanese": "他の目標の数を表示", "korean": "다른 목표 개수 표시", "chinese": "显示其他目标数量",
        "russian": "Показывать число других целей", "german": "Anzahl weiterer Ziele anzeigen",
        "french": "Afficher le nombre d'autres objectifs", "spanish": "Mostrar el recuento de otros objetivos",
        "italian": "Mostra il conteggio degli altri obiettivi", "polish": "Pokaż liczbę pozostałych celów",
        "czech": "Zobrazit počet dalších cílů",
    },
    "CNO_AngleToShowMarkerDetails": {
        "japanese": "マーカー詳細を表示する角度", "korean": "마커 세부 정보를 표시할 각도", "chinese": "显示标记详情的角度",
        "russian": "Угол показа сведений о маркере", "german": "Winkel für Markierungsdetails",
        "french": "Angle d'affichage des détails du marqueur", "spanish": "Ángulo para mostrar detalles del marcador",
        "italian": "Angolo per mostrare i dettagli dell'indicatore", "polish": "Kąt pokazywania szczegółów znacznika",
        "czech": "Úhel zobrazení podrobností značky",
    },
    "CNO_HelpAngleToShowMarkerDetails": {
        "japanese": "マーカーの名前と距離が表示されるまでに、コンパスの中心にどれだけ近づく必要があるか。",
        "korean": "마커의 이름과 거리가 표시되기까지 나침반 중앙에 얼마나 가까워져야 하는지입니다.",
        "chinese": "标记需要多接近罗盘中心,其名称与距离才会显示。",
        "russian": "Насколько близко к центру компаса должен находиться маркер, чтобы появились его имя и расстояние.",
        "german": "Wie nah eine Markierung an der Mitte des Kompasses sein muss, bevor ihr Name und ihre Entfernung erscheinen.",
        "french": "À quelle distance du centre de la boussole un marqueur doit se trouver avant que son nom et sa distance apparaissent.",
        "spanish": "Qué tan cerca del centro de la brújula debe estar un marcador antes de que aparezcan su nombre y distancia.",
        "italian": "Quanto un indicatore deve essere vicino al centro della bussola prima che compaiano il suo nome e la sua distanza.",
        "polish": "Jak blisko środka kompasu musi znajdować się znacznik, zanim pojawi się jego nazwa i odległość.",
        "czech": "Jak blízko středu kompasu musí být značka, než se zobrazí její název a vzdálenost.",
    },
    "CNO_AngleToKeepMarkerDetailsShown": {
        "japanese": "マーカー詳細を表示し続ける角度", "korean": "마커 세부 정보를 계속 표시할 각도", "chinese": "保持标记详情显示的角度",
        "russian": "Угол сохранения сведений о маркере", "german": "Winkel zum Beibehalten der Markierungsdetails",
        "french": "Angle de maintien des détails du marqueur", "spanish": "Ángulo para mantener los detalles del marcador",
        "italian": "Angolo per mantenere visibili i dettagli dell'indicatore", "polish": "Kąt utrzymania szczegółów znacznika",
        "czech": "Úhel udržení podrobností značky",
    },
    "CNO_HelpAngleToKeepMarkerDetailsShown": {
        "japanese": "一度表示されると、この広い角度を超えて外れるまでマーカーの詳細は表示され続けます - しきい値付近でのちらつきを防ぎます。",
        "korean": "한 번 표시되면 이 더 넓은 각도를 벗어날 때까지 마커의 세부 정보가 계속 표시됩니다 - 임계값 근처에서 깜박이는 것을 방지합니다.",
        "chinese": "一旦显示,标记的详情会一直保持可见,直到偏离这个更宽的角度为止——防止在临界值附近闪烁。",
        "russian": "После появления сведения о маркере остаются видимыми, пока он не выйдет за пределы этого более широкого угла - предотвращает мерцание текста прямо на границе.",
        "german": "Sobald angezeigt, bleiben die Markierungsdetails sichtbar, bis sie über diesen weiteren Winkel hinausdriften - verhindert Flackern genau an der Schwelle.",
        "french": "Une fois affichés, les détails du marqueur restent visibles jusqu'à ce qu'il dérive au-delà de cet angle plus large - évite le scintillement juste au seuil.",
        "spanish": "Una vez mostrados, los detalles del marcador permanecen visibles hasta que se aleja más allá de este ángulo más amplio - evita el parpadeo justo en el umbral.",
        "italian": "Una volta mostrati, i dettagli dell'indicatore restano visibili finché non supera questo angolo più ampio - evita lo sfarfallio proprio alla soglia.",
        "polish": "Po pokazaniu szczegóły znacznika pozostają widoczne, dopóki nie wyjdzie poza ten szerszy kąt - zapobiega migotaniu tuż przy progu.",
        "czech": "Jakmile se zobrazí, podrobnosti značky zůstanou viditelné, dokud se nedostane za tento širší úhel - zabraňuje blikání těsně na hranici.",
    },
    "CNO_FocusingDelayToShow": {
        "japanese": "表示までのフォーカス遅延", "korean": "표시까지의 포커스 지연", "chinese": "显示前的聚焦延迟",
        "russian": "Задержка фокусировки перед показом", "german": "Fokusverzögerung bis zur Anzeige",
        "french": "Délai de mise au point avant affichage", "spanish": "Retardo de enfoque antes de mostrar",
        "italian": "Ritardo di messa a fuoco prima della visualizzazione", "polish": "Opóźnienie skupienia przed pokazaniem",
        "czech": "Zpoždění zaostření před zobrazením",
    },
    "CNO_HelpFocusingDelayToShow": {
        "japanese": "詳細が表示されるまでに、マーカーが上の角度内にとどまる必要がある時間。",
        "korean": "세부 정보가 표시되기 전까지 마커가 위 각도 내에 머물러야 하는 시간입니다.",
        "chinese": "标记必须在上述角度内停留多久,其详情才会显示。",
        "russian": "Сколько времени маркер должен оставаться в пределах указанного выше угла, прежде чем появятся сведения о нём.",
        "german": "Wie lange eine Markierung innerhalb des obigen Winkels bleiben muss, bevor ihre Details erscheinen.",
        "french": "Combien de temps un marqueur doit rester dans l'angle ci-dessus avant que ses détails apparaissent.",
        "spanish": "Cuánto tiempo debe permanecer un marcador dentro del ángulo anterior antes de que aparezcan sus detalles.",
        "italian": "Per quanto tempo un indicatore deve rimanere entro l'angolo sopra indicato prima che compaiano i suoi dettagli.",
        "polish": "Jak długo znacznik musi pozostawać w powyższym kącie, zanim pojawią się jego szczegóły.",
        "czech": "Jak dlouho musí značka zůstat v uvedeném úhlu, než se zobrazí její podrobnosti.",
    },
    "CNO_QuestListTitle": {
        "japanese": "クエストリスト", "korean": "퀘스트 목록", "chinese": "任务列表", "russian": "Список заданий",
        "german": "Questliste", "french": "Liste de quêtes", "spanish": "Lista de misiones",
        "italian": "Elenco missioni", "polish": "Lista zadań", "czech": "Seznam úkolů",
    },
    "CNO_PositionX": {
        "japanese": "位置X", "korean": "위치 X", "chinese": "位置 X", "russian": "Позиция X", "german": "Position X",
        "french": "Position X", "spanish": "Posición X", "italian": "Posizione X", "polish": "Pozycja X", "czech": "Pozice X",
    },
    "CNO_PositionY": {
        "japanese": "位置Y", "korean": "위치 Y", "chinese": "位置 Y", "russian": "Позиция Y", "german": "Position Y",
        "french": "Position Y", "spanish": "Posición Y", "italian": "Posizione Y", "polish": "Pozycja Y", "czech": "Pozice Y",
    },
    "CNO_MaxHeight": {
        "japanese": "最大高さ", "korean": "최대 높이", "chinese": "最大高度", "russian": "Макс. высота",
        "german": "Maximale Höhe", "french": "Hauteur maximale", "spanish": "Altura máxima",
        "italian": "Altezza massima", "polish": "Maksymalna wysokość", "czech": "Maximální výška",
    },
    "CNO_ShowInExteriors": {
        "japanese": "屋外で表示", "korean": "실외에서 표시", "chinese": "在室外显示", "russian": "Показывать снаружи",
        "german": "Im Freien anzeigen", "french": "Afficher en extérieur", "spanish": "Mostrar en exteriores",
        "italian": "Mostra negli esterni", "polish": "Pokaż na zewnątrz", "czech": "Zobrazit venku",
    },
    "CNO_ShowInInteriors": {
        "japanese": "屋内で表示", "korean": "실내에서 표시", "chinese": "在室内显示", "russian": "Показывать внутри",
        "german": "In Innenräumen anzeigen", "french": "Afficher en intérieur", "spanish": "Mostrar en interiores",
        "italian": "Mostra negli interni", "polish": "Pokaż wewnątrz", "czech": "Zobrazit uvnitř",
    },
    "CNO_HideInCombat": {
        "japanese": "戦闘中は非表示", "korean": "전투 중 숨기기", "chinese": "战斗中隐藏", "russian": "Скрывать в бою",
        "german": "Im Kampf ausblenden", "french": "Masquer en combat", "spanish": "Ocultar en combate",
        "italian": "Nascondi in combattimento", "polish": "Ukryj podczas walki", "czech": "Skrýt v boji",
    },
    "CNO_HelpHideInCombat": {
        "japanese": "武器や呪文を構えている間、クエストリストを完全に非表示にします。",
        "korean": "무기나 주문을 꺼낸 상태에서는 퀘스트 목록을 완전히 숨깁니다.",
        "chinese": "在拔出武器或法术时完全隐藏任务列表。",
        "russian": "Полностью скрывает список заданий, пока оружие или заклинание наготове.",
        "german": "Blendet die Questliste vollständig aus, während eine Waffe oder ein Zauber gezogen ist.",
        "french": "Masque entièrement la liste de quêtes lorsqu'une arme ou un sort est dégainé.",
        "spanish": "Oculta por completo la lista de misiones mientras un arma o hechizo está desenvainado.",
        "italian": "Nasconde completamente l'elenco missioni mentre un'arma o un incantesimo è impugnato.",
        "polish": "Całkowicie ukrywa listę zadań, gdy dobyta jest broń lub zaklęcie.",
        "czech": "Zcela skryje seznam úkolů, dokud je tažena zbraň nebo kouzlo.",
    },
    "CNO_WalkingDelayToShow": {
        "japanese": "歩行時の表示遅延", "korean": "걷기 표시 지연", "chinese": "步行显示延迟", "russian": "Задержка показа при ходьбе",
        "german": "Anzeigeverzögerung beim Gehen", "french": "Délai d'affichage en marchant", "spanish": "Retardo de aparición al caminar",
        "italian": "Ritardo di visualizzazione camminando", "polish": "Opóźnienie pokazywania podczas chodzenia",
        "czech": "Zpoždění zobrazení při chůzi",
    },
    "CNO_JoggingDelayToShow": {
        "japanese": "ジョギング時の表示遅延", "korean": "조깅 표시 지연", "chinese": "慢跑显示延迟", "russian": "Задержка показа при беге трусцой",
        "german": "Anzeigeverzögerung beim Joggen", "french": "Délai d'affichage en trottinant", "spanish": "Retardo de aparición al trotar",
        "italian": "Ritardo di visualizzazione mentre si fa jogging", "polish": "Opóźnienie pokazywania podczas truchtu",
        "czech": "Zpoždění zobrazení při klusu",
    },
    "CNO_SprintingDelayToShow": {
        "japanese": "スプリント時の表示遅延", "korean": "질주 표시 지연", "chinese": "冲刺显示延迟", "russian": "Задержка показа при спринте",
        "german": "Anzeigeverzögerung beim Sprinten", "french": "Délai d'affichage en sprintant", "spanish": "Retardo de aparición al esprintar",
        "italian": "Ritardo di visualizzazione durante lo scatto", "polish": "Opóźnienie pokazywania podczas sprintu",
        "czech": "Zpoždění zobrazení při sprintu",
    },
    "CNO_HelpPaceDelays": {
        "japanese": "クエストリストがフェードインするまでに、それぞれのペースでどれだけ移動し続ける必要があるか。",
        "korean": "퀘스트 목록이 서서히 나타나기 전까지 각 이동 속도로 얼마나 오래 이동해야 하는지입니다.",
        "chinese": "在任务列表淡入之前,你需要以每种步伐移动多长时间。",
        "russian": "Сколько нужно двигаться в каждом темпе, прежде чем список заданий постепенно появится.",
        "german": "Wie lange du dich in jedem Tempo bewegen musst, bevor die Questliste einblendet.",
        "french": "Combien de temps vous devez vous déplacer à chaque allure avant que la liste de quêtes apparaisse en fondu.",
        "spanish": "Cuánto tiempo debes moverte a cada ritmo antes de que la lista de misiones aparezca gradualmente.",
        "italian": "Per quanto tempo devi muoverti a ciascuna andatura prima che l'elenco missioni compaia in dissolvenza.",
        "polish": "Jak długo musisz poruszać się w każdym tempie, zanim lista zadań się pojawi.",
        "czech": "Jak dlouho se musíte pohybovat každým tempem, než se seznam úkolů zobrazí.",
    },
    "CNO_Debug": {
        "japanese": "デバッグ", "korean": "디버그", "chinese": "调试", "russian": "Отладка", "german": "Debug",
        "french": "Débogage", "spanish": "Depuración", "italian": "Debug", "polish": "Debugowanie", "czech": "Ladění",
    },
    "CNO_LogLevel": {
        "japanese": "ログレベル", "korean": "로그 레벨", "chinese": "日志级别", "russian": "Уровень журнала",
        "german": "Protokollstufe", "french": "Niveau de journal", "spanish": "Nivel de registro",
        "italian": "Livello di log", "polish": "Poziom logowania", "czech": "Úroveň logování",
    },
    "CNO_LogLevel_Trace": {
        "japanese": "トレース", "korean": "추적", "chinese": "跟踪", "russian": "Трассировка", "german": "Trace",
        "french": "Trace", "spanish": "Trace", "italian": "Trace", "polish": "Trace", "czech": "Trace",
    },
    "CNO_LogLevel_Debug": {
        "japanese": "デバッグ", "korean": "디버그", "chinese": "调试", "russian": "Отладка", "german": "Debug",
        "french": "Débogage", "spanish": "Depuración", "italian": "Debug", "polish": "Debugowanie", "czech": "Ladění",
    },
    "CNO_LogLevel_Info": {
        "japanese": "情報", "korean": "정보", "chinese": "信息", "russian": "Информация", "german": "Info",
        "french": "Infos", "spanish": "Información", "italian": "Informazioni", "polish": "Informacje", "czech": "Informace",
    },
    "CNO_LogLevel_Warning": {
        "japanese": "警告", "korean": "경고", "chinese": "警告", "russian": "Предупреждение", "german": "Warnung",
        "french": "Avertissement", "spanish": "Advertencia", "italian": "Avviso", "polish": "Ostrzeżenie", "czech": "Varování",
    },
    "CNO_LogLevel_Error": {
        "japanese": "エラー", "korean": "오류", "chinese": "错误", "russian": "Ошибка", "german": "Fehler",
        "french": "Erreur", "spanish": "Error", "italian": "Errore", "polish": "Błąd", "czech": "Chyba",
    },
    "CNO_LogLevel_Critical": {
        "japanese": "重大", "korean": "치명적", "chinese": "严重", "russian": "Критическая",
        "german": "Kritisch", "french": "Critique", "spanish": "Crítico", "italian": "Critico",
        "polish": "Krytyczny", "czech": "Kritická",
    },
    "CNO_LogLevel_Off": {
        "japanese": "オフ", "korean": "끄기", "chinese": "关闭", "russian": "Отключено", "german": "Aus",
        "french": "Désactivé", "spanish": "Desactivado", "italian": "Disattivato", "polish": "Wyłączone", "czech": "Vypnuto",
    },
    "CNO_HelpLogLevel": {
        "japanese": "ログに即座に適用されます。",
        "korean": "로그에 즉시 적용됩니다.",
        "chinese": "立即应用于日志。",
        "russian": "Применяется к журналу немедленно.",
        "german": "Wird sofort auf das Log angewendet.",
        "french": "S'applique immédiatement au journal.",
        "spanish": "Se aplica de inmediato al registro.",
        "italian": "Si applica immediatamente al log.",
        "polish": "Stosowane natychmiast do logu.",
        "czech": "Použije se okamžitě na log.",
    },
    "CNO_SaveBtn": {
        "japanese": "保存", "korean": "저장", "chinese": "保存", "russian": "Сохранить", "german": "Speichern",
        "french": "Enregistrer", "spanish": "Guardar", "italian": "Salva", "polish": "Zapisz", "czech": "Uložit",
    },
    "CNO_StatusSaved": {
        "japanese": "設定を保存しました。", "korean": "설정을 저장했습니다.", "chinese": "设置已保存。",
        "russian": "Настройки сохранены.", "german": "Einstellungen gespeichert.", "french": "Paramètres enregistrés.",
        "spanish": "Ajustes guardados.", "italian": "Impostazioni salvate.", "polish": "Ustawienia zapisane.",
        "czech": "Nastavení uložena.",
    },
    "CNO_StatusSaveFail": {
        "japanese": "INIの保存に失敗しました。理由はログを確認してください。",
        "korean": "INI를 저장할 수 없습니다. 이유는 로그를 확인하세요.",
        "chinese": "无法保存 INI。请查看日志了解原因。",
        "russian": "Не удалось сохранить INI. Причина — в журнале.",
        "german": "Die INI konnte nicht gespeichert werden. Der Grund steht im Log.",
        "french": "Impossible d'enregistrer l'INI. Voyez le journal pour la raison.",
        "spanish": "No se pudo guardar el INI. Consulta el registro para saber por qué.",
        "italian": "Impossibile salvare l'INI. Consulta il log per il motivo.",
        "polish": "Nie można zapisać INI. Sprawdź log, aby dowiedzieć się dlaczego.",
        "czech": "Nelze uložit INI. Důvod najdete v logu.",
    },
    "CNO_HelpSave": {
        "japanese": "上記のすべての設定をプラグインのINIに書き込みます。コメントと無関係なキーはそのまま残ります。",
        "korean": "위의 모든 설정을 플러그인의 INI에 다시 기록합니다. 주석과 관련 없는 키는 그대로 유지됩니다.",
        "chinese": "将以上所有设置写回插件的 INI。注释和无关的键保持不变。",
        "russian": "Записывает каждую настройку выше обратно в INI плагина. Комментарии и посторонние ключи остаются без изменений.",
        "german": "Schreibt jede obige Einstellung zurück in die INI des Plugins. Kommentare und nicht verwandte Schlüssel bleiben unverändert.",
        "french": "Réécrit chaque paramètre ci-dessus dans le fichier INI du plugin. Les commentaires et les clés non liées restent inchangés.",
        "spanish": "Vuelve a escribir cada ajuste anterior en el INI del plugin. Los comentarios y las claves no relacionadas quedan intactos.",
        "italian": "Riscrive ogni impostazione sopra nell'INI del plugin. I commenti e le chiavi non correlate restano intatti.",
        "polish": "Zapisuje każde powyższe ustawienie z powrotem do pliku INI wtyczki. Komentarze i niezwiązane klucze pozostają bez zmian.",
        "czech": "Zapíše každé výše uvedené nastavení zpět do INI pluginu. Komentáře a nesouvisející klíče zůstanou nezměněny.",
    },
    "CNO_ReloadBtn": {
        "japanese": "INIから再読み込み", "korean": "INI에서 다시 불러오기", "chinese": "从 INI 重新加载",
        "russian": "Перезагрузить из INI", "german": "Aus INI neu laden", "french": "Recharger depuis l'INI",
        "spanish": "Recargar desde el INI", "italian": "Ricarica dall'INI", "polish": "Wczytaj ponownie z INI",
        "czech": "Znovu načíst z INI",
    },
    "CNO_StatusReloaded": {
        "japanese": "INIから設定を再読み込みしました。", "korean": "INI에서 설정을 다시 불러왔습니다.",
        "chinese": "已从 INI 重新加载设置。", "russian": "Настройки перезагружены из INI.",
        "german": "Einstellungen aus der INI neu geladen.", "french": "Paramètres rechargés depuis l'INI.",
        "spanish": "Ajustes recargados desde el INI.", "italian": "Impostazioni ricaricate dall'INI.",
        "polish": "Ustawienia wczytane ponownie z INI.", "czech": "Nastavení znovu načtena z INI.",
    },
    "CNO_StatusReloadFail": {
        "japanese": "INIの読み込みに失敗しました。理由はログを確認してください。",
        "korean": "INI를 읽을 수 없습니다. 이유는 로그를 확인하세요.",
        "chinese": "无法读取 INI。请查看日志了解原因。",
        "russian": "Не удалось прочитать INI. Причина — в журнале.",
        "german": "Die INI konnte nicht gelesen werden. Der Grund steht im Log.",
        "french": "Impossible de lire l'INI. Voyez le journal pour la raison.",
        "spanish": "No se pudo leer el INI. Consulta el registro para saber por qué.",
        "italian": "Impossibile leggere l'INI. Consulta il log per il motivo.",
        "polish": "Nie można odczytać INI. Sprawdź log, aby dowiedzieć się dlaczego.",
        "czech": "Nelze přečíst INI. Důvod najdete v logu.",
    },
    "CNO_HelpReload": {
        "japanese": "最後の保存以降にここで行った変更をすべて捨て、ディスクからINIを再読み込みします。ファイルを手動で編集した変更も取り込みます。",
        "korean": "마지막 저장 이후 여기서 만든 변경 사항을 모두 버리고 디스크에서 INI를 다시 읽습니다. 파일을 직접 편집한 내용도 반영됩니다.",
        "chinese": "放弃自上次保存以来在此处所做的任何更改,并从磁盘重新读取 INI。也会读取手动编辑该文件所做的更改。",
        "russian": "Отбрасывает все изменения, сделанные здесь с последнего сохранения, и заново считывает INI с диска. Также подхватывает изменения, сделанные вручную в файле.",
        "german": "Verwirft jede hier seit dem letzten Speichern vorgenommene Änderung und liest die INI erneut von der Festplatte. Übernimmt auch von Hand vorgenommene Änderungen an der Datei.",
        "french": "Annule tout changement effectué ici depuis le dernier enregistrement et relit l'INI depuis le disque. Reprend aussi les modifications faites manuellement dans le fichier.",
        "spanish": "Descarta cualquier cambio hecho aquí desde el último guardado y vuelve a leer el INI desde el disco. También recoge los cambios hechos a mano en el archivo.",
        "italian": "Scarta ogni modifica fatta qui dall'ultimo salvataggio e rilegge l'INI dal disco. Recupera anche le modifiche fatte a mano al file.",
        "polish": "Odrzuca wszelkie zmiany wprowadzone tutaj od ostatniego zapisu i ponownie odczytuje INI z dysku. Uwzględnia też zmiany wprowadzone ręcznie w pliku.",
        "czech": "Zahodí všechny změny provedené zde od posledního uložení a znovu načte INI z disku. Zohlední i změny provedené ručně v souboru.",
    },
    "CNO_RestoreBtn": {
        "japanese": "既定値に戻す", "korean": "기본값으로 복원", "chinese": "恢复默认值", "russian": "Восстановить умолч.",
        "german": "Standard wiederherstellen", "french": "Restaurer les valeurs par défaut",
        "spanish": "Restaurar valores predeterminados", "italian": "Ripristina i valori predefiniti",
        "polish": "Przywróć wartości domyślne", "czech": "Obnovit výchozí",
    },
    "CNO_StatusRestored": {
        "japanese": "既定値に戻しました。保存を押して確定してください。",
        "korean": "기본값으로 복원했습니다. 유지하려면 저장을 누르세요.",
        "chinese": "已恢复默认值。按保存以保留它们。",
        "russian": "Значения по умолчанию восстановлены. Нажмите «Сохранить», чтобы закрепить их.",
        "german": "Standardwerte wiederherstellt. Drücke Speichern, um sie zu behalten.",
        "french": "Valeurs par défaut restaurées. Appuyez sur Enregistrer pour les conserver.",
        "spanish": "Valores predeterminados restaurados. Pulsa Guardar para conservarlos.",
        "italian": "Valori predefiniti ripristinati. Premi Salva per conservarli.",
        "polish": "Przywrócono wartości domyślne. Naciśnij Zapisz, aby je zachować.",
        "czech": "Výchozí hodnoty obnoveny. Stiskněte Uložit, abyste je zachovali.",
    },
    "CNO_HelpRestore": {
        "japanese": "すべての設定を新規インストール時の値に戻します。保存ボタンを押すまで何も書き込まれません。",
        "korean": "모든 설정을 새로 설치했을 때의 값으로 되돌립니다. 저장 버튼을 누르기 전까지는 아무것도 기록되지 않습니다.",
        "chinese": "将每个设置恢复为全新安装时的值。在你按下保存按钮之前,不会写入任何内容。",
        "russian": "Возвращает каждую настройку к значению, которое было бы при свежей установке. Ничего не записывается, пока вы не нажмёте кнопку «Сохранить».",
        "german": "Setzt jede Einstellung auf den Wert zurück, den sie bei einer frischen Installation hätte. Nichts wird geschrieben, bis du auf die Schaltfläche Speichern drückst.",
        "french": "Remet chaque paramètre à sa valeur d'une installation neuve. Rien n'est écrit avant que vous n'appuyiez sur le bouton Enregistrer.",
        "spanish": "Devuelve cada ajuste al valor que tendría en una instalación nueva. No se escribe nada hasta que pulses el botón Guardar.",
        "italian": "Riporta ogni impostazione al valore che avrebbe in un'installazione nuova. Non viene scritto nulla finché non premi il pulsante Salva.",
        "polish": "Przywraca każde ustawienie do wartości z nowej instalacji. Nic nie zostaje zapisane, dopóki nie naciśniesz przycisku Zapisz.",
        "czech": "Vrátí každé nastavení na hodnotu, jakou by mělo při čerstvé instalaci. Nic se nezapíše, dokud nestisknete tlačítko Uložit.",
    },
    "CNO_Intro": {
        "japanese": "ほとんどの設定は変更するとすぐに適用されます。次回プレイ時にも残すには保存を押してください。",
        "korean": "대부분의 설정은 변경하는 즉시 적용됩니다. 다음에 플레이할 때도 유지하려면 저장을 누르세요.",
        "chinese": "大多数设置在你更改后会立即生效。按保存可在下次游玩时保留它们。",
        "russian": "Большинство настроек применяются сразу же после изменения. Нажмите «Сохранить», чтобы они остались и в следующий раз.",
        "german": "Die meisten Einstellungen wirken sofort, sobald du sie änderst. Drücke Speichern, um sie für das nächste Mal zu behalten.",
        "french": "La plupart des paramètres s'appliquent dès que vous les modifiez. Appuyez sur Enregistrer pour les garder la prochaine fois.",
        "spanish": "La mayoría de los ajustes se aplican en cuanto los cambias. Pulsa Guardar para conservarlos la próxima vez que juegues.",
        "italian": "La maggior parte delle impostazioni si applica non appena le modifichi. Premi Salva per conservarle per la prossima partita.",
        "polish": "Większość ustawień obowiązuje natychmiast po ich zmianie. Naciśnij Zapisz, aby zachować je na następną rozgrywkę.",
        "czech": "Většina nastavení se použije okamžitě po jejich změně. Stiskněte Uložit, abyste je zachovali pro příští hraní.",
    },
}


def write_translation_file(path, entries):
    lines = []
    for key, text in entries.items():
        escaped = text.replace("\r\n", "\n").replace("\n", "\\n")
        lines.append(f"${key}\t{escaped}")
    body = "\r\n".join(lines) + "\r\n"
    data = b"\xff\xfe" + body.encode("utf-16-le")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def main():
    keys, order = read_keys()
    missing_translation_keys = [k for k in order if k not in TRANSLATIONS]
    if missing_translation_keys:
        raise RuntimeError(f"no translations held for keys found in source: {missing_translation_keys}")
    extra_translation_keys = [k for k in TRANSLATIONS if k not in keys]
    if extra_translation_keys:
        raise RuntimeError(f"translations held for keys no longer in source: {extra_translation_keys}")

    out_dir = os.path.join(REPO, "dist", "Interface", "Translations")
    english = {k: keys[k] for k in order}
    write_translation_file(os.path.join(out_dir, "CompassNavigationOverhaul_english.txt"), english)
    print(f"english: {len(english)} keys")

    for lang in LANGS[1:]:
        translated = {k: TRANSLATIONS[k][lang] for k in order}
        write_translation_file(os.path.join(out_dir, f"CompassNavigationOverhaul_{lang}.txt"), translated)
        print(f"{lang}: {len(translated)} keys written")


if __name__ == "__main__":
    main()
