# IP9 LG Remote

Pilot do telewizorów LG webOS (Wi-Fi / IR / Bluetooth / USB, air mouse) dla M5Stack Cardputer i Cardputer ADV.
Część marki IP9 (https://cardputer.ip9.pl), repozytorium `IP9LGRemote`.

## Wspólny rdzeń IP9Kit

- Powłoka aplikacji, ekrany list / karuzeli / pomocy / dialogów, klawiatura, LED, bateria i podświetlenie
  pochodzą z biblioteki **IP9Kit** (osobne repozytorium, `../IP9Kit`). `App` dziedziczy z `ip9::Shell`.
- `platformio.ini`: `[ip9kit] source` przypina tag IP9Kit. Praca nad rdzeniem razem z pilotem:
  skopiować `ip9kit-local.ini.example` do `ip9kit-local.ini` (git-ignored) → `symlink://../IP9Kit`.
- `src/ui/Screen.h` wprowadza `using namespace ip9;` do `lgremote` i definiuje klasy `Screen` / `ListScreen`
  aplikacji (z `app_`). Wskaźniki „dowolny ekran na stosie” to `ip9::Screen*`.
- Ekran bez własnego `chipLabel()` pokazuje w pasku statusu tryb łącza; `trait::kInfraredOnly` (w `traits()`)
  wymusza tryb IR w pasku i na LED.
- Co jest wspólne dla wielu aplikacji – do IP9Kit (z nową wersją i tagiem), nie tutaj.

## Zasady

- Kod w 100% po angielsku; teksty UI w `src/ui/Strings.h`, wspólne stopki w IP9Kit (`ip9/ui/Strings.h`).
  `python tools/check_text_fit.py` (po `pio run`) sprawdza, czy teksty mieszczą się na ekranie.
- Nazwy: produkt „IP9 LG Remote”, obrazy `IP9LGRemote-<wersja>[-full].bin` (`custom_release_name`).
- Nie zmieniać: przestrzeni NVS ustawień, ścieżki kopii `/config/lgtv-remote/settings.json`,
  nazwy Bluetooth „Cardputer Remote” – zmiana zgubiłaby ustawienia lub parowanie użytkowników.
- Publikacja: `publish/M5BURNER.md`; okładki `python tools/make_cover.py`, zrzuty `python tools/make_screenshots.py`.
