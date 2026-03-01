# Hướng Dẫn Sử Dụng StockMaster

Tài liệu này dành cho người dùng cuối, mô tả các thao tác chính đang chạy được trong app theo đúng code hiện tại.

## 1) Tổng quan nhanh

StockMaster hiện hỗ trợ các nhóm nghiệp vụ sau:
- quản lý khách hàng
- quản lý sản phẩm
- quản lý lô hàng theo hạn dùng
- nhập/xuất và điều chỉnh tồn
- tạo đơn bán hàng dạng draft rồi xác nhận
- lập phiếu thu và theo dõi công nợ
- xem báo cáo và export
- kiểm tra và tải bản cập nhật phần mềm

Thanh điều hướng bên trái gồm:
- `Tổng quan`
- `Khách hàng`
- `Sản phẩm`
- `Đơn hàng`
- `Tồn kho`
- `Thanh toán`
- `Báo cáo`
- `Cài đặt`

## 2) Nguyên tắc sử dụng quan trọng

- Mọi ngày nhập theo định dạng `YYYY-MM-DD`.
- `Khách hàng` và `mã khách` được tự sinh, không nhập tay.
- `Đơn hàng` là luồng bán hàng chính; khi `xác nhận đơn`, hệ thống mới trừ kho.
- `Phiếu thu` là luồng riêng trong `Thanh toán`; dùng để thu tiền sau khi đơn đã được xác nhận.
- Dữ liệu được lưu vào SQLite local, nên tắt app rồi mở lại vẫn còn dữ liệu.

## 3) Quy trình chuẩn từ đầu đến cuối

Luồng dùng điển hình:
1. Tạo khách hàng
2. Tạo sản phẩm
3. Thêm lô hàng cho sản phẩm
4. Nếu cần, nhập thêm/xuất thử theo lô
5. Tạo đơn hàng draft
6. Thêm mặt hàng vào đơn
7. Xác nhận đơn để trừ kho
8. Lập phiếu thu
9. Xem báo cáo và đối soát

## 4) Thêm khách hàng

Vào `Khách hàng`.

Thao tác:
1. Nhập tên khách.
2. Nhập số điện thoại.
3. Nhập địa chỉ.
4. Nhập hạn mức công nợ nếu cần.
5. Bấm nút tạo/lưu.

Lưu ý:
- Mã khách được tạo tự động sau khi lưu.
- Có thể chọn khách trong danh sách bên trái để sửa hoặc xóa.
- Ô tìm kiếm bên trái dùng để lọc nhanh theo mã/tên khách.
- Sau khi lưu khách mới, màn `Đơn hàng` và `Thanh toán` sẽ tự dùng danh sách khách mới khi bạn chuyển sang các màn đó.

## 5) Thêm sản phẩm

Vào `Sản phẩm`.

Thao tác tạo sản phẩm:
1. Kiểm tra mã sản phẩm hiển thị ở trạng thái `Tự sinh khi lưu`.
2. Nhập tên sản phẩm.
3. Nhập đơn vị tính.
4. Nhập giá bán sỉ mặc định.
5. Bấm tạo/lưu.

Lưu ý:
- Mã sản phẩm được tự sinh khi tạo mới, không cần nhập tay.
- Chọn sản phẩm ở danh sách bên trái để sửa hoặc xóa.
- Danh sách bên trái hiển thị luôn tồn tổng và số lô của từng sản phẩm.

## 6) Thêm lô hàng ban đầu cho sản phẩm

Sau khi đã tạo sản phẩm, vẫn trong `Sản phẩm`, chọn sản phẩm cần thao tác.

Ở khối quản lý lô:
1. Nhập mã lô.
2. Nhập ngày nhập `YYYY-MM-DD`.
3. Nhập hạn dùng `YYYY-MM-DD`.
4. Nhập số lượng ban đầu của lô.
5. Bấm thêm lô.

Lưu ý:
- `Hạn dùng` không được nhỏ hơn `Ngày nhập`.
- App sẽ hiển thị cảnh báo nếu lô còn `<= 30 ngày` là hết hạn.
- Nếu lô đã quá hạn, app hiển thị badge `Quá hạn`.

## 7) Nhập thêm hàng hoặc xuất hàng theo lô

Sau khi có lô, trong `Sản phẩm`:

### Nhập thêm cho lô
1. Chọn sản phẩm.
2. Chọn đúng lô trong danh sách lô.
3. Ở khối nhập/xuất, nhập số lượng nhập.
4. Nhập ngày thao tác.
5. Bấm `Nhập`.

### Xuất trực tiếp khỏi lô
1. Chọn sản phẩm.
2. Chọn đúng lô.
3. Nhập số lượng xuất.
4. Nhập ngày thao tác.
5. Bấm `Xuất`.

Lưu ý:
- Không thể xuất vượt số lượng tồn của lô.
- Mọi thao tác nhập/xuất sẽ được ghi vào lịch sử biến động tồn.

## 8) Điều chỉnh tồn có lý do

Vào `Tồn kho`.

Mục này dùng khi cần chỉnh tồn thủ công do kiểm kê, hỏng hàng, lệch số liệu.

Thao tác:
1. Chọn sản phẩm trong danh sách bên trái.
2. Chọn lô cần điều chỉnh.
3. Nhập số lượng điều chỉnh:
   - số dương: cộng tồn
   - số âm: trừ tồn
4. Nhập lý do.
5. Nhập ngày điều chỉnh.
6. Bấm thực hiện điều chỉnh.

Lưu ý:
- Không cho phép điều chỉnh âm vượt quá tồn hiện có.
- Sau khi điều chỉnh, phần `Lịch sử biến động` sẽ ghi lại lý do.

## 9) Tạo hóa đơn bán hàng (đơn hàng)

Trong app hiện tại, phần “hóa đơn” vận hành theo `Đơn hàng`.

Vào `Đơn hàng`.

### Bước 1: Tạo draft
1. Mở tab nhỏ `Tạo Draft`.
2. Nếu cần, gõ tên khách hoặc số điện thoại để tìm nhanh.
3. Chọn đúng khách trong danh sách đã lọc.
4. Bấm tạo draft mới.

### Bước 2: Gán khách cho draft
1. Chuyển sang tab `Chi tiết đơn`.
2. Chọn khách hàng cho đơn.
3. Lưu cập nhật nếu có thay đổi.

### Bước 3: Thêm mặt hàng vào đơn
1. Chuyển sang tab `Item & Xử lý`.
2. Chọn sản phẩm.
3. Chọn lô ưu tiên để xuất trước.
4. Nhập số lượng.
5. Kiểm tra khối preview phân bổ:
   - nếu lô đang chọn không đủ, app sẽ tự lấy tiếp từ lô khác
6. Nhập đơn giá nếu cần chỉnh so với giá mặc định.
7. Bấm thêm/cập nhật item.

### Bước 4: Xác nhận đơn
1. Kiểm tra lại tổng tiền và các dòng hàng.
2. Bấm `Xác nhận`.

Khi xác nhận:
- hệ thống trừ kho theo allocation từ các lô hiện có
- đơn chuyển sang trạng thái đã ghi nhận doanh số
- đơn sẽ xuất hiện trong luồng công nợ/thanh toán

Lưu ý:
- Chỉ draft mới sửa được item hoặc đổi khách.
- Nếu không đủ tồn, app sẽ chặn xác nhận.
- Nếu cần hủy sau khi đã xác nhận, dùng `Void`; hệ thống sẽ hoàn kho.
- Sau khi đơn đã `Confirmed`, có thể bấm `Sang thanh toán` để mở ngay màn thu tiền cho đúng khách/đúng đơn.

## 10) Tìm lại đơn hàng đã tạo

Ở cột trái của `Đơn hàng`:

Có thể lọc theo:
- mã đơn
- một ngày cụ thể
- khoảng ngày

Thao tác:
1. Nhập mã đơn hoặc ngày.
2. Bấm `Lọc`.
3. Dùng `Xóa lọc` để quay về danh sách đầy đủ.

## 11) Lập phiếu thu và thu tiền

Vào `Thanh toán`.

Thao tác:
1. Chọn khách ở danh sách bên trái.
2. Xem phần công nợ hiện tại ở panel bên phải.
3. Trong khối `Lập phiếu thu`, chọn đơn còn công nợ.
4. Nhập số tiền thu.
5. Nhập phương thức thu tiền.
6. Nhập ngày thu.
7. Bấm tạo phiếu thu.

Kết quả:
- hệ thống tự chặn `over-payment`
- thu một phần thì đơn chuyển `PartiallyPaid`
- thu đủ thì đơn chuyển `Paid`
- lịch sử phiếu thu và ledger đối soát sẽ cập nhật ngay

## 12) Xem dashboard để vận hành hằng ngày

Vào `Tổng quan`.

Các phần nên xem:
- số khách hàng
- số sản phẩm
- số đơn mở
- số mặt hàng sắp hết
- số mặt hàng có lô sắp hết hạn
- công nợ phải thu
- cảnh báo vận hành
- biểu đồ doanh số và thu tiền 7 ngày gần nhất
- top khách hàng và top sản phẩm

Cách dùng thực tế:
- buổi sáng: xem `Cảnh báo vận hành`
- cuối ngày/cuối tuần: xem biểu đồ doanh số và thu tiền
- rê chuột vào từng cột trong biểu đồ để xem đúng giá trị tiền của `Doanh số` hoặc `Thu tiền`

## 13) Xem báo cáo và export

Vào `Báo cáo`.

### Sales Summary Theo Kỳ
Dùng để xem:
- số đơn
- doanh số
- số tiền đã thu
- còn phải thu

Thao tác:
1. Nhập `Từ ngày` và `Đến ngày`.
2. Bấm tải lại dữ liệu nếu cần.
3. Bấm export `CSV` hoặc `PDF`.

### Debt Aging Report
Dùng để xem công nợ theo tuổi nợ:
- `0-30 ngày`
- `31-60 ngày`
- `>60 ngày`

### Inventory Movement
Dùng để tra cứu lịch sử biến động tồn:
- theo từ khóa SKU/tên/lô/lý do
- theo ngày hoặc khoảng ngày

Lưu ý:
- File export mặc định nằm trong thư mục `Documents/StockMasterExports`.

## 14) Cập nhật phần mềm

Vào `Cài đặt`.

Thao tác đúng:
1. Bấm `Kiểm tra phiên bản`.
2. Nếu có bản phù hợp nền tảng, nút `Cập nhật` sẽ được bật.
3. Bấm `Cập nhật`.
4. App tải gói cập nhật rồi tự mở gói cài đặt.

Lưu ý:
- Việc cập nhật không làm mất dữ liệu cũ.
- Lý do: SQLite nằm trong thư mục dữ liệu riêng của người dùng, không nằm trong gói cài đặt.

## 15) Các lỗi thường gặp và cách xử lý

### Không xác nhận được đơn
Nguyên nhân thường gặp:
- chưa chọn khách
- chưa có item
- không đủ tồn

Cách xử lý:
- quay lại kiểm tra tồn theo lô ở `Sản phẩm` hoặc `Tồn kho`

### Không tạo được phiếu thu
Nguyên nhân thường gặp:
- chưa chọn khách
- đơn chưa ở trạng thái cho phép thu
- số tiền thu lớn hơn công nợ còn lại

### Không thêm được lô
Nguyên nhân thường gặp:
- sai định dạng ngày
- hạn dùng nhỏ hơn ngày nhập

## 16) Khuyến nghị vận hành

Để hạn chế sai lệch dữ liệu, nên dùng theo thứ tự:
1. Tạo sản phẩm và lô trước khi bán
2. Chỉ xác nhận đơn khi đã kiểm tra đủ tồn
3. Dùng `Thanh toán` để thu tiền, không tự sửa trạng thái đơn
4. Dùng `Tồn kho` cho điều chỉnh kiểm kê, luôn ghi rõ lý do
5. Cuối ngày xem `Báo cáo` và `Tổng quan` để đối soát
