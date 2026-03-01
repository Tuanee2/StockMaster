# UI Guidelines — StockMaster (macOS style)

Tài liệu này định nghĩa **design system** để UI của StockMaster có phong cách **macOS** nhất quán trên mọi screen.

> Mục tiêu: nhìn “như app macOS”, sạch, tối giản, spacing chuẩn, tương tác nhẹ nhàng.

---

## 1) Principles

- **Minimal**: ít đường viền, ít màu, tập trung nội dung.
- **Depth nhẹ**: shadow mỏng + blur lớn (không đổ bóng gắt).
- **Typography rõ**: ưu tiên SF Pro (macOS) nếu có; fallback system.
- **Motion tinh tế**: hover/focus/press chỉ thay đổi nhẹ (opacity/scale).

---

## 2) Layout

### App Shell
- **Sidebar trái** cố định (giống Finder / Settings).
- **Top toolbar** nhẹ (nút action chính nằm đây).
- **Content area** bên phải: dùng card/list/table.

### Spacing (khuyến nghị)
- Page padding: **20–24 px**
- Card padding: **16–20 px**
- Item spacing: **8–12 px**
- Section spacing: **16–24 px**

---

## 3) Visual tokens

### Corner radius
- Card / panel: **12 px**
- Button: **10 px**
- Input: **10 px**
- Small chips/tag: **8 px**

### Border
- Border 1px rất mờ (tuỳ theme):
  - Dark: `#FFFFFF14`
  - Light: `#00000014`

### Shadow (subtle)
- Không dùng shadow dày.
- Blur lớn, opacity thấp.

> Gợi ý QML: `layer.enabled: true` + DropShadow (Qt Graphical Effects) hoặc hiệu ứng tương đương.

---

## 4) Colors

StockMaster hỗ trợ **Dark-first** (macOS Sonoma vibe). Có thể thêm Light theme sau.

### Dark theme (default)
- Background: `#1C1C1E`
- Surface/Card: `#2C2C2E`
- Surface elevated: `#3A3A3C`
- Text primary: `#FFFFFF`
- Text secondary: `#A1A1A6`
- Divider/border: `#FFFFFF14`
- Danger: `#FF453A`
- Success: `#32D74B`

### Accent
- Dùng “system accent” nếu có, hoặc accent mặc định: `#0A84FF`.

---

## 5) Typography

- Font family:
  - macOS: `"SF Pro Display"` / `"SF Pro Text"` (fallback `system`)
- Base size: **13–14**
- Section title: **16–18** (SemiBold)
- Page title: **22–26** (Bold)

---

## 6) Interaction rules

### Hover
- Đổi nền rất nhẹ (surface -> surface elevated) hoặc tăng opacity 5–8%.

### Press
- Scale **0.98–0.99** hoặc giảm opacity 8–12%.

### Focus
- Ring mảnh 2px với accent opacity thấp.

### Animations
- Duration: **120–220 ms**
- Easing: OutCubic/OutQuad
- Tránh animation quá dài.

---

## 7) Components (BẮT BUỘC dùng lại)

Agent khi làm UI phải ưu tiên tạo & dùng component trong:
`src/stockmaster/ui/qml/components/`

- `MacButton.qml`
- `MacIconButton.qml`
- `MacCard.qml`
- `MacTextField.qml`
- `MacTable.qml` (wrapper cho TableView style)
- `MacSidebar.qml`
- `MacToolbar.qml`
- `MacDialog.qml` / `MacSheet.qml`
- `MacToast.qml`

Nguyên tắc:
- **Không** dùng style mặc định của Qt Quick Controls (Fusion/Material).
- **Không** hardcode style trong từng screen.

---

## 8) Screen templates

### List screen (Customers/Products/Orders)
- Toolbar: search + filter + primary action (New)
- Body: table/list + pagination (nếu cần)

### Detail screen
- Header: title + status pill + actions
- Body: sections bằng card

---

## 9) Iconography

- Dùng icon đơn sắc (monochrome), line icons.
- Prefer SF Symbols (nếu dùng được), hoặc bộ icon tương đương.
- Kích thước phổ biến: 16 / 18 / 20.

---

## 10) Accessibility

- Contrast đủ (text secondary vẫn đọc được).
- Hit target: tối thiểu 28–32 px cho button/icon.
- Keyboard navigation (desktop): Tab focus cho input/action chính.

---

## 11) Definition of done (UI)

Một screen “xong” khi:
- Đúng layout (sidebar + toolbar + content).
- Dùng component chuẩn (không style rải rác).
- Không block UI khi load list.
- Có empty/loading/error states.