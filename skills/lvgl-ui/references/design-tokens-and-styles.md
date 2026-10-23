# 设计令牌与 LVGL 样式

## 令牌表

先维护一张书面令牌表，再编写页面：

| 类别 | 示例令牌 | 用途 |
|---|---|---|
| 背景 | `color_bg` | 页面底色 |
| 表面 | `color_surface` | 卡片、信息栏 |
| 文本 | `color_text_primary` | 主文本 |
| 弱化文本 | `color_text_muted` | 辅助信息 |
| 强调 | `color_accent` | 主操作与选中态 |
| 危险 | `color_danger` | 删除、错误、重置 |
| 间距 | `space_1/2/3` | 4/8/12 px 等固定间距 |
| 圆角 | `radius_card/button` | 同类控件统一圆角 |
| 字体 | `font_body/font_title` | 业务语义字体 |

尺寸令牌要保留具体 px；不要用模糊的 `small/large` 代替实际约束。

## 样式注册表

每个项目建立唯一的 `ui_theme.c/.h` 或同义模块，集中初始化 `lv_style_t`：

```c
static lv_style_t style_btn_primary;
static lv_style_t style_btn_primary_pressed;
static lv_style_transition_dsc_t style_transition_btn;

static const lv_style_prop_t btn_transition_props[] = {
    LV_STYLE_BG_COLOR,
    LV_STYLE_BORDER_COLOR,
    LV_STYLE_TEXT_COLOR,
    0,
};

void ui_theme_init(void)
{
    lv_style_init(&style_btn_primary);
    lv_style_set_bg_color(&style_btn_primary, color_accent);
    lv_style_set_radius(&style_btn_primary, radius_button);
    lv_style_set_text_color(&style_btn_primary, color_on_accent);

    lv_style_transition_dsc_init(
        &style_transition_btn,
        btn_transition_props,
        lv_anim_path_ease_out,
        120,
        0,
        NULL);
    lv_style_set_transition(&style_btn_primary_pressed,
                            &style_transition_btn);
    lv_style_set_bg_color(&style_btn_primary_pressed, color_accent_dark);
}

void ui_style_apply_button_primary(lv_obj_t *button)
{
    lv_obj_add_style(button, &style_btn_primary, LV_PART_MAIN);
    lv_obj_add_style(button,
                     &style_btn_primary_pressed,
                     LV_PART_MAIN | LV_STATE_PRESSED);
}
```

业务页面通过 `ui_style_apply_*()` 使用样式，不重复写完整视觉属性。

## 允许的页面级样式变更

以下情况可以由页面或动画回调设置样式，但应封装成有语义的函数：

- 动画进度驱动的透明度、位移和颜色插值；
- 业务数据驱动的进度条、图表或状态颜色；
- 同一组件在不同页面上的几何布局；
- 临时高亮、错误反馈和恢复动画。

不得以“只是一行”为理由在多个页面复制相同的背景、边框、字体或状态规则。

## XML 与 LVGL Pro

- 使用可复用的 `style_` 样式对象，而不是在每个组件节点写内联属性。
- 页面只声明结构、绑定和语义状态。
- 状态差异由 `LV_STATE_*` 选择器表达。
- 生成 C/H 文件不手工修改；持久修改必须回到 XML/Pro 源文件。

## 审查方法

1. 搜索 `lv_obj_set_style_`，区分布局/动画与重复视觉定义。
2. 搜索颜色字面量和 `lv_color_hex`，确认它们只存在于令牌定义处。
3. 搜索同类控件创建函数，确认 pressed/focused/disabled/checked 都使用同一套样式。
4. 删除单个样式后检查资源是否仍有引用，避免不可达资源。
