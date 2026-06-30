# Mios Map Editor (MME)
*Ein moderner, kollaborativer und shader-gestützter Map-Editor für OpenTibia (OTBM).*
MME ist ein ambitionierter Fork des legendären Remere's Map Editor (RME), der speziell für moderne Ansprüche und verbesserte Workflows entwickelt wurde. Anstatt nur den alten Code zu verwalten, modernisiert MME die Engine im Kern und führt zukunftsweisende Features für Map-Designer ein.
### 🌟 Key Features
*   **👥 Live-Kollaboration (Multiplayer-Mapping):** Arbeite dank integrierter Client/Server-Architektur in Echtzeit mit anderen Mappern gleichzeitig an derselben Map.
*   **🎨 Next-Gen Rendering & Shader:** Optische Aufwertung durch moderne Grafik-Pipelines. MME unterstützt Deferred Rendering, Global Illumination (GI) Raytracing, CRT-Retro-Filter und Weichzeichnungseffekte (Blur) direkt im Editor.
*   **⚡ Modernste C++ Architektur:**
    *   **EnTT (Entity Component System):** Extrem schnelle Speicher- und Entityverwaltung für flüssiges Mapping auch auf gigantischen Maps.
    *   **Dear ImGui Integration:** Flexibles, modernes UI-Docking-System für eine aufgeräumte, anpassbare Arbeitsumgebung.
    *   **Vector Graphics (NanoVG/NanoSVG):** Skalierbare, gestochen scharfe Vektor-Benutzeroberflächen.
*   **🎲 Prozedurale Generierung:** Nutze integrierte Rauschfunktionen (*FastNoiseLite*) zur schnellen Erzeugung von organischen Landschaften und Höhlen.
*   **📦 Einfaches Bauen via VCPKG:** Vollständige Integration von Microsofts Paketmanager für unkomplizierte Builds auf Windows, Linux und macOS.
