const canvas = document.getElementById('screenCanvas');
const ctx = canvas.getContext('2d');
const propertiesContent = document.getElementById('propertiesContent');
const rectList = document.getElementById('rectList');

let rectangles = [];
let selectedIndex = -1;
let isDragging = false;
let dragTarget = null; // 'icon', 'icon_roll', 'text', 'text_roll'
let dragItemIndex = -1; // 被拖动的元素索引
let startX, startY;

// Simple single-step undo snapshot
let _lastSnapshot = null;

function pushUndoSnapshot() {
    try {
        _lastSnapshot = JSON.parse(JSON.stringify({ rectangles, selectedIndex, alignmentRef }));
    } catch (e) {
        // ignore snapshot errors
        _lastSnapshot = null;
    }
}

function undoOnce() {
    if (!_lastSnapshot) {
        return;
    }
    try {
        const s = JSON.parse(JSON.stringify(_lastSnapshot));
        rectangles = s.rectangles || [];
        selectedIndex = (s.selectedIndex !== undefined) ? s.selectedIndex : -1;
        alignmentRef = s.alignmentRef || null;
        _lastSnapshot = null; // single-step
        render();
        updateRectList();
        showProperties();
    } catch (e) {
        console.error('undo failed', e);
    }
}

let alignmentRef = null; // { x: number, y: number, type: string, rectIndex: number, itemIndex: number }
let isSnappingEnabled = true;

const SCREEN_WIDTH = 416;
const SCREEN_HEIGHT = 240;

// Initialize with some data
function init() {
    rectangles = [
        {
            index: 0,
            is_mother: "non",
            Group: [],
            x_: 0, y_: 0, width_: 0.8, height_: 0.12,
            focus_mode: 0, focus_icon: "horn",
            on_confirm_action: "",
            icon_count: 0,
            icons: [], 
            icon_roll: [], 
            text_count: 0,
            texts: [], 
            text_roll: []
        }
    ];
    render();
    updateRectList();
}

function render() {
    ctx.clearRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Draw grid
    ctx.strokeStyle = '#e5e7eb';
    ctx.lineWidth = 1;
    for(let i=0; i<SCREEN_WIDTH; i+=20) {
        ctx.beginPath(); ctx.moveTo(i, 0); ctx.lineTo(i, SCREEN_HEIGHT); ctx.stroke();
    }
    for(let i=0; i<SCREEN_HEIGHT; i+=20) {
        ctx.beginPath(); ctx.moveTo(0, i); ctx.lineTo(SCREEN_WIDTH, i); ctx.stroke();
    }

    // Draw alignment lines
    if (alignmentRef) {
        ctx.beginPath();
        ctx.setLineDash([5, 5]);
        ctx.strokeStyle = '#10b981'; // Emerald 500
        ctx.lineWidth = 1;
        
        // Vertical line
        ctx.moveTo(alignmentRef.x, 0);
        ctx.lineTo(alignmentRef.x, SCREEN_HEIGHT);
        
        // Horizontal line
        ctx.moveTo(0, alignmentRef.y);
        ctx.lineTo(SCREEN_WIDTH, alignmentRef.y);
        
        ctx.stroke();
        ctx.setLineDash([]);
        
        // Draw a small indicator at the reference point
        ctx.beginPath();
        ctx.arc(alignmentRef.x, alignmentRef.y, 3, 0, Math.PI * 2);
        ctx.fillStyle = '#10b981';
        ctx.fill();
    }

    rectangles.forEach((rect, i) => {
        const x = rect.x_ * SCREEN_WIDTH;
        const y = rect.y_ * SCREEN_HEIGHT;
        const w = rect.width_ * SCREEN_WIDTH;
        const h = rect.height_ * SCREEN_HEIGHT;

        // Draw connection to children if it's a mom
        if (rect.is_mother === 'mom' && rect.Group && Array.isArray(rect.Group)) {
            rect.Group.forEach(childIdx => {
                const child = rectangles.find(r => r.index === childIdx || rectangles.indexOf(r) === childIdx);
                if (child) {
                    ctx.beginPath();
                    ctx.setLineDash([2, 2]);
                    ctx.moveTo(x + w/2, y + h/2);
                    ctx.lineTo(child.x_ * SCREEN_WIDTH + (child.width_ * SCREEN_WIDTH)/2, child.y_ * SCREEN_HEIGHT + (child.height_ * SCREEN_HEIGHT)/2);
                    ctx.strokeStyle = 'rgba(59, 130, 246, 0.4)';
                    ctx.stroke();
                    ctx.setLineDash([]);
                }
            });
        }

        // Draw Rect
        ctx.fillStyle = i === selectedIndex ? 'rgba(59, 130, 246, 0.1)' : 'rgba(0, 0, 0, 0.02)';
        ctx.fillRect(x, y, w, h);
        
        ctx.strokeStyle = i === selectedIndex ? '#3b82f6' : '#9ca3af';
        ctx.lineWidth = i === selectedIndex ? 2 : 1;
        ctx.strokeRect(x, y, w, h);

        // Draw Label
        ctx.fillStyle = i === selectedIndex ? '#3b82f6' : '#4b5563';
        ctx.font = 'bold 10px sans-serif';
        let role = 'NON';
        if (rect.is_mother === 'mom') role = 'MOM';
        else if (rect.is_mother === 'son') role = 'SON';
        ctx.fillText(`${role} ${i}`, x + 5, y + 15);

        // Draw Icons
        if (rect.icons) {
            rect.icons.forEach((icon, ii) => {
                const ix = x + (icon.rel_x * w);
                const iy = y + (icon.rel_y * h);
                
                // 绘制可拖动区域（半透明圆形）
                ctx.beginPath(); 
                ctx.arc(ix, iy, 8, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(239, 68, 68, 0.1)'; 
                ctx.fill();
                
                // 绘制图标中心点
                ctx.beginPath(); 
                ctx.arc(ix, iy, 4, 0, Math.PI * 2);
                ctx.fillStyle = '#ef4444'; 
                ctx.fill();
                
                ctx.font = '9px sans-serif'; 
                ctx.fillStyle = '#ef4444';
                ctx.fillText(icon.icon_name || `icon${ii}`, ix + 10, iy + 3);
            });
        }

        // Draw Icon Roll
        if (rect.icon_roll) {
            rect.icon_roll.forEach((roll, ri) => {
                const rx = x + (roll.rel_x * w);
                const ry = y + (roll.rel_y * h);
                
                // 绘制可拖动区域
                ctx.beginPath(); 
                ctx.rect(rx - 8, ry - 8, 16, 16);
                ctx.fillStyle = 'rgba(245, 158, 11, 0.1)';
                ctx.fill();
                
                // 绘制图标边框
                ctx.beginPath(); 
                ctx.rect(rx - 4, ry - 4, 8, 8);
                ctx.strokeStyle = '#f59e0b'; 
                ctx.lineWidth = 2;
                ctx.stroke();
                
                ctx.fillStyle = '#f59e0b'; 
                ctx.font = '9px sans-serif';
                ctx.fillText(`roll:${roll.icon_arr}`, rx + 10, ry + 3);
            });
        }

        // Draw Static Texts
        if (rect.texts) {
            rect.texts.forEach((text, ti) => {
                const tx = x + (text.rel_x * w);
                const ty = y + (text.rel_y * h);
                
                // 绘制可拖动区域
                ctx.beginPath();
                ctx.arc(tx, ty, 8, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(59, 130, 246, 0.1)';
                ctx.fill();
                
                // 绘制文本标记
                ctx.fillStyle = '#3b82f6'; 
                ctx.font = 'bold 10px sans-serif';
                ctx.fillText("T", tx - 3, ty + 3);
                
                ctx.font = '9px sans-serif'; 
                ctx.fillText(`type:${text.type}`, tx + 10, ty + 3);
            });
        }

        // Draw Text Roll
        if (rect.text_roll) {
            rect.text_roll.forEach((text, ti) => {
                const tx = x + (text.rel_x * w);
                const ty = y + (text.rel_y * h);
                
                // 绘制可拖动区域
                ctx.beginPath();
                ctx.arc(tx, ty, 8, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(16, 185, 129, 0.1)';
                ctx.fill();
                
                // 绘制文本标记
                ctx.fillStyle = '#10b981'; 
                ctx.fillRect(tx - 3, ty - 3, 6, 6);
                
                ctx.font = '9px sans-serif'; 
                ctx.fillText(text.text_arr || `text${ti}`, tx + 10, ty + 3);
            });
        }
    });
}

function updateRectList() {
    rectList.innerHTML = '';
    rectangles.forEach((rect, i) => {
        const div = document.createElement('div');
        div.className = `rect-item p-2 text-sm cursor-pointer rounded border ${i === selectedIndex ? 'active-rect' : 'border-transparent'}`;
        div.innerHTML = `Rect ${i}: ${rect.is_mother} (${Math.round(rect.width_*100)}% x ${Math.round(rect.height_*100)}%)`;
        div.onclick = () => selectRect(i);
        rectList.appendChild(div);
    });
}

function selectRect(index) {
    selectedIndex = index;
    updateRectList();
    render();
    showProperties();
}

function showProperties() {
    if (selectedIndex === -1) {
        propertiesContent.innerHTML = '<p class="text-gray-500 italic">Select a rectangle to edit its properties</p>';
        return;
    }

    const rect = rectangles[selectedIndex];
    propertiesContent.innerHTML = `
        <div class="space-y-4">
            <div class="flex justify-between items-center bg-emerald-50 p-2 rounded border border-emerald-100">
                <span class="text-xs font-bold text-emerald-700">Alignment Tools</span>
                <button onclick="clearAlignment()" class="text-xs text-emerald-600 hover:underline">Clear Lines</button>
            </div>

            <div>
                <label class="block text-xs font-medium text-gray-500 uppercase">Role (Non/Mom/Son)</label>
                <select onchange="updateProp('is_mother', this.value)" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm font-bold">
                    <option value="non" ${rect.is_mother === 'non' ? 'selected' : ''}>NON (independent)</option>
                    <option value="mom" ${rect.is_mother === 'mom' ? 'selected' : ''}>MOM (mother)</option>
                    <option value="son" ${rect.is_mother === 'son' ? 'selected' : ''}>SON (child)</option>
                </select>
            </div>
            
            <div>
                <label class="block text-xs font-medium text-gray-500 uppercase">Group (Child Indices, e.g. 1,2)</label>
                <input type="text" value="${(rect.Group || []).join(',')}" onchange="updateProp('Group', this.value.split(',').map(v => parseInt(v.trim())).filter(v => !isNaN(v)))" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm" placeholder="Indices of children">
            </div>

            <div class="grid grid-cols-2 gap-4">
                <div>
                    <label class="block text-xs font-medium text-gray-500 uppercase">X (0-1)</label>
                    <input type="number" step="0.01" value="${rect.x_}" oninput="updateProp('x_', parseFloat(this.value))" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm">
                </div>
                <div>
                    <label class="block text-xs font-medium text-gray-500 uppercase">Y (0-1)</label>
                    <input type="number" step="0.01" value="${rect.y_}" oninput="updateProp('y_', parseFloat(this.value))" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm">
                </div>
                <div>
                    <label class="block text-xs font-medium text-gray-500 uppercase">Width (0-1)</label>
                    <input type="number" step="0.01" value="${rect.width_}" oninput="updateProp('width_', parseFloat(this.value))" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm">
                </div>
                <div>
                    <label class="block text-xs font-medium text-gray-500 uppercase">Height (0-1)</label>
                    <input type="number" step="0.01" value="${rect.height_}" oninput="updateProp('height_', parseFloat(this.value))" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm">
                </div>
            </div>
            <div class="grid grid-cols-2 gap-4">
                <div>
                    <label class="block text-xs font-medium text-gray-500 uppercase">Focus Mode</label>
                    <input type="number" value="${rect.focus_mode || 0}" oninput="updateProp('focus_mode', parseInt(this.value))" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm">
                </div>
                <div>
                    <label class="block text-xs font-medium text-gray-500 uppercase">Focus Icon</label>
                    <input type="text" value="${rect.focus_icon || ''}" oninput="updateProp('focus_icon', this.value)" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm">
                </div>
            </div>

            <div>
                <label class="block text-xs font-medium text-gray-500 uppercase">Confirm Action</label>
                <input type="text" value="${rect.on_confirm_action || ''}" oninput="updateProp('on_confirm_action', this.value)" class="mt-1 block w-full border-gray-300 rounded-md shadow-sm text-sm">
            </div>

            <!-- Icons Management -->
            <div class="border-t pt-4">
                <div class="flex justify-between items-center mb-2">
                    <label class="block text-xs font-bold text-gray-700 uppercase">Icons (${rect.icons?.length || 0})</label>
                    <button onclick="addIcon()" class="text-blue-600 text-xs hover:underline">+ Add Icon</button>
                </div>
                <div class="space-y-2">
                    ${(rect.icons || []).map((icon, i) => `
                        <div class="bg-gray-50 p-2 rounded text-xs space-y-1 border border-gray-200">
                            <div class="flex justify-between">
                                <input type="text" value="${icon.icon_name}" oninput="updateIcon(${i}, 'icon_name', this.value)" class="w-24 border-none bg-transparent font-bold p-0 focus:ring-0" placeholder="icon_name">
                                <div>
                                    <button onclick="setAlignmentRef(${selectedIndex}, ${i}, 'icon')" class="text-emerald-600 mr-2" title="Set Alignment Reference">⚓</button>
                                    <button onclick="removeIcon(${i})" class="text-red-500">×</button>
                                </div>
                            </div>
                            <div class="grid grid-cols-2 gap-1">
                                <span>X: <input type="number" step="0.01" value="${icon.rel_x}" oninput="updateIcon(${i}, 'rel_x', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                                <span>Y: <input type="number" step="0.01" value="${icon.rel_y}" oninput="updateIcon(${i}, 'rel_y', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                            </div>
                        </div>
                    `).join('')}
                </div>
            </div>

            <!-- Icon Roll Management -->
            <div class="border-t pt-4">
                <div class="flex justify-between items-center mb-2">
                    <label class="block text-xs font-bold text-gray-700 uppercase">Icon Roll (${rect.icon_roll?.length || 0})</label>
                    <button onclick="addIconRoll()" class="text-orange-600 text-xs hover:underline">+ Add Roll</button>
                </div>
                <div class="space-y-2">
                    ${(rect.icon_roll || []).map((roll, i) => `
                        <div class="bg-gray-50 p-2 rounded text-xs space-y-1 border border-gray-200">
                            <div class="flex justify-between">
                                <input type="text" value="${roll.icon_arr || ''}" oninput="updateIconRoll(${i}, 'icon_arr', this.value)" class="w-24 border-none bg-transparent font-bold p-0 focus:ring-0" placeholder="icon_arr">
                                <div>
                                    <button onclick="setAlignmentRef(${selectedIndex}, ${i}, 'icon_roll')" class="text-emerald-600 mr-2" title="Set Alignment Reference">⚓</button>
                                    <button onclick="removeIconRoll(${i})" class="text-red-500">×</button>
                                </div>
                            </div>
                            <div class="space-y-1">
                                <div>Idx: <input type="text" value="${roll.idx || ''}" oninput="updateIconRoll(${i}, 'idx', this.value)" class="w-full border-gray-300 rounded p-1" placeholder="$status_idx"></div>
                                <div class="grid grid-cols-2 gap-1">
                                    <span>X: <input type="number" step="0.01" value="${roll.rel_x}" oninput="updateIconRoll(${i}, 'rel_x', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                                    <span>Y: <input type="number" step="0.01" value="${roll.rel_y}" oninput="updateIconRoll(${i}, 'rel_y', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                                </div>
                                <label class="flex items-center">
                                    <input type="checkbox" ${roll.auto_roll ? 'checked' : ''} onchange="updateIconRoll(${i}, 'auto_roll', this.checked)" class="mr-1"> Auto Roll
                                </label>
                            </div>
                        </div>
                    `).join('')}
                </div>
            </div>

            <!-- Static Texts Management -->
            <div class="border-t pt-4">
                <div class="flex justify-between items-center mb-2">
                    <label class="block text-xs font-bold text-gray-700 uppercase">Static Texts (${rect.texts?.length || 0})</label>
                    <button onclick="addStaticText()" class="text-indigo-600 text-xs hover:underline">+ Add Text</button>
                </div>
                <div class="space-y-2">
                    ${(rect.texts || []).map((text, i) => `
                        <div class="bg-gray-50 p-2 rounded text-xs space-y-1 border border-gray-200">
                            <div class="flex justify-between">
                                <select onchange="updateStaticText(${i}, 'type', parseInt(this.value))" class="border-none bg-transparent p-0 focus:ring-0 font-bold">
                                    <option value="0" ${text.type === 0 ? 'selected' : ''}>WORD</option>
                                    <option value="1" ${text.type === 1 ? 'selected' : ''}>PHONETIC</option>
                                    <option value="2" ${text.type === 2 ? 'selected' : ''}>DEF</option>
                                </select>
                                <div>
                                    <button onclick="setAlignmentRef(${selectedIndex}, ${i}, 'text')" class="text-emerald-600 mr-2" title="Set Alignment Reference">⚓</button>
                                    <button onclick="removeStaticText(${i})" class="text-red-500">×</button>
                                </div>
                            </div>
                            <div class="grid grid-cols-2 gap-1">
                                <span>X: <input type="number" step="0.01" value="${text.rel_x}" oninput="updateStaticText(${i}, 'rel_x', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                                <span>Y: <input type="number" step="0.01" value="${text.rel_y}" oninput="updateStaticText(${i}, 'rel_y', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                            </div>
                            <div class="grid grid-cols-3 gap-1">
                                <span>Size: <input type="number" value="${text.font_size || 12}" oninput="updateStaticText(${i}, 'font_size', parseInt(this.value))" class="w-12 border-gray-300 rounded p-0"></span>
                                <span>H: <input type="number" value="${text.h_align !== undefined ? text.h_align : 1}" oninput="updateStaticText(${i}, 'h_align', parseInt(this.value))" class="w-8 border-gray-300 rounded p-0"></span>
                                <span>V: <input type="number" value="${text.v_align !== undefined ? text.v_align : 1}" oninput="updateStaticText(${i}, 'v_align', parseInt(this.value))" class="w-8 border-gray-300 rounded p-0"></span>
                            </div>
                        </div>
                    `).join('')}
                </div>
            </div>

            <!-- Text Roll Management -->
            <div class="border-t pt-4">
                <div class="flex justify-between items-center mb-2">
                    <label class="block text-xs font-bold text-gray-700 uppercase">Text Roll (${rect.text_roll?.length || 0})</label>
                    <button onclick="addText()" class="text-green-600 text-xs hover:underline">+ Add Roll</button>
                </div>
                <div class="space-y-2">
                    ${(rect.text_roll || []).map((text, i) => `
                        <div class="bg-gray-50 p-2 rounded text-xs space-y-1 border border-gray-200">
                            <div class="flex justify-between">
                                <input type="text" value="${text.text_arr || ''}" oninput="updateText(${i}, 'text_arr', this.value)" class="w-24 border-none bg-transparent font-bold p-0 focus:ring-0" placeholder="text_arr">
                                <div>
                                    <button onclick="setAlignmentRef(${selectedIndex}, ${i}, 'text_roll')" class="text-emerald-600 mr-2" title="Set Alignment Reference">⚓</button>
                                    <button onclick="removeText(${i})" class="text-red-500">×</button>
                                </div>
                            </div>
                            <div class="space-y-1">
                                <div>Idx: <input type="text" value="${text.idx || ''}" oninput="updateText(${i}, 'idx', this.value)" class="w-full border-gray-300 rounded p-1" placeholder="$message_idx"></div>
                                <div class="grid grid-cols-2 gap-1">
                                    <span>X: <input type="number" step="0.01" value="${text.rel_x}" oninput="updateText(${i}, 'rel_x', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                                    <span>Y: <input type="number" step="0.01" value="${text.rel_y}" oninput="updateText(${i}, 'rel_y', parseFloat(this.value))" class="w-12 border-none bg-transparent p-0 focus:ring-0"></span>
                                </div>
                                <label class="flex items-center">
                                    <input type="checkbox" ${text.auto_roll ? 'checked' : ''} onchange="updateText(${i}, 'auto_roll', this.checked)" class="mr-1"> Auto Roll
                                </label>
                            </div>
                        </div>
                    `).join('')}
                </div>
            </div>

            <button onclick="deleteRect(${selectedIndex})" class="w-full bg-red-50 hover:bg-red-100 text-red-600 py-2 rounded border border-red-200 text-sm transition">Delete Rectangle</button>
        </div>
    `;
}

window.updateProp = (prop, val) => {
    if (selectedIndex !== -1) {
        pushUndoSnapshot();
        rectangles[selectedIndex][prop] = val;
        render();
        updateRectList();
    }
};

window.setAlignmentRef = (rectIdx, itemIdx, type) => {
    const rect = rectangles[rectIdx];
    const rx = rect.x_ * SCREEN_WIDTH;
    const ry = rect.y_ * SCREEN_HEIGHT;
    const rw = rect.width_ * SCREEN_WIDTH;
    const rh = rect.height_ * SCREEN_HEIGHT;
    
    let item;
    if (type === 'icon') item = rect.icons[itemIdx];
    else if (type === 'icon_roll') item = rect.icon_roll[itemIdx];
    else if (type === 'text') item = rect.texts[itemIdx];
    else if (type === 'text_roll') item = rect.text_roll[itemIdx];
    
    if (item) {
        alignmentRef = {
            x: rx + (item.rel_x * rw),
            y: ry + (item.rel_y * rh),
            type: type,
            rectIndex: rectIdx,
            itemIndex: itemIdx
        };
        render();
        showProperties();
    }
};

window.clearAlignment = () => {
    alignmentRef = null;
    render();
    showProperties();
};

window.addIcon = () => {
    pushUndoSnapshot();
    const rect = rectangles[selectedIndex];
    if (!rect.icons) rect.icons = [];
    rect.icons.push({ icon_name: 'nail', rel_x: 0.5, rel_y: 0.5 });
    showProperties(); render();
};

window.updateIcon = (i, prop, val) => {
    pushUndoSnapshot();
    rectangles[selectedIndex].icons[i][prop] = val;
    render();
};

window.removeIcon = (i) => {
    pushUndoSnapshot();
    if (alignmentRef && alignmentRef.rectIndex === selectedIndex && alignmentRef.itemIndex === i && alignmentRef.type === 'icon') {
        alignmentRef = null;
    }
    rectangles[selectedIndex].icons.splice(i, 1);
    showProperties(); render();
};

window.addIconRoll = () => {
    pushUndoSnapshot();
    const rect = rectangles[selectedIndex];
    if (!rect.icon_roll) rect.icon_roll = [];
    rect.icon_roll.push({ icon_arr: 'status_icons', idx: '$status_idx', rel_x: 0.5, rel_y: 0.5, auto_roll: false });
    showProperties(); render();
};

window.updateIconRoll = (i, prop, val) => {
    pushUndoSnapshot();
    rectangles[selectedIndex].icon_roll[i][prop] = val;
    render();
};

window.removeIconRoll = (i) => {
    pushUndoSnapshot();
    if (alignmentRef && alignmentRef.rectIndex === selectedIndex && alignmentRef.itemIndex === i && alignmentRef.type === 'icon_roll') {
        alignmentRef = null;
    }
    rectangles[selectedIndex].icon_roll.splice(i, 1);
    showProperties(); render();
};

window.addStaticText = () => {
    pushUndoSnapshot();
    const rect = rectangles[selectedIndex];
    if (!rect.texts) rect.texts = [];
    rect.texts.push({ rel_x: 0.5, rel_y: 0.5, type: 0, font_size: 12, h_align: 1, v_align: 1 });
    showProperties(); render();
};

window.updateStaticText = (i, prop, val) => {
    pushUndoSnapshot();
    rectangles[selectedIndex].texts[i][prop] = val;
    render();
};

window.removeStaticText = (i) => {
    pushUndoSnapshot();
    if (alignmentRef && alignmentRef.rectIndex === selectedIndex && alignmentRef.itemIndex === i && alignmentRef.type === 'text') {
        alignmentRef = null;
    }
    rectangles[selectedIndex].texts.splice(i, 1);
    showProperties(); render();
};

window.addText = () => {
    pushUndoSnapshot();
    const rect = rectangles[selectedIndex];
    if (!rect.text_roll) rect.text_roll = [];
    rect.text_roll.push({ text_arr: 'message_remind', idx: '$message_idx', rel_x: 0.5, rel_y: 0.5, auto_roll: false });
    showProperties(); render();
};

window.updateText = (i, prop, val) => {
    pushUndoSnapshot();
    rectangles[selectedIndex].text_roll[i][prop] = val;
    render();
};

window.removeText = (i) => {
    pushUndoSnapshot();
    if (alignmentRef && alignmentRef.rectIndex === selectedIndex && alignmentRef.itemIndex === i && alignmentRef.type === 'text_roll') {
        alignmentRef = null;
    }
    rectangles[selectedIndex].text_roll.splice(i, 1);
    showProperties(); render();
};

window.deleteRect = (index) => {
    pushUndoSnapshot();
    if (alignmentRef && alignmentRef.rectIndex === index) {
        alignmentRef = null;
    } else if (alignmentRef && alignmentRef.rectIndex > index) {
        alignmentRef.rectIndex--; // 更新索引
    }
    rectangles.splice(index, 1);
    selectedIndex = -1;
    render();
    updateRectList();
    showProperties();
};

document.getElementById('addRectBtn').onclick = () => {
    pushUndoSnapshot();
    rectangles.push({
        index: rectangles.length,
        is_mother: "non",
        Group: [],
        x_: 0.1, y_: 0.1, width_: 0.2, height_: 0.2,
        focus_mode: 0, focus_icon: "nail",
        on_confirm_action: "",
        icon_count: 0,
        icons: [], 
        icon_roll: [], 
        text_count: 0,
        texts: [], 
        text_roll: []
    });
    selectRect(rectangles.length - 1);
};

document.getElementById('exportJsonBtn').onclick = () => {
    const data = {
        rect_count: rectangles.length,
        rectangles: rectangles.map((r, i) => {
            const result = {
                index: i,
                is_mother: r.is_mother || "non"
            };
            
            // 只有 mom 才需要 Group
            if (r.is_mother === 'mom') {
                result.Group = r.Group || [];
            }
            
            // 位置和尺寸
            result.x_ = parseFloat((r.x_ || 0).toFixed(3));
            result.y_ = parseFloat((r.y_ || 0).toFixed(3));
            result.width_ = parseFloat((r.width_ || 0.1).toFixed(3));
            result.height_ = parseFloat((r.height_ || 0.1).toFixed(3));
            
            // Focus相关
            result.focus_mode = r.focus_mode || 0;
            result.focus_icon = r.focus_icon || "nail";
            result.on_confirm_action = r.on_confirm_action || "open_menu";
            
            // Icons
            result.icon_count = r.icons ? r.icons.length : 0;
            result.icons = r.icons || [];
            result.icon_roll = r.icon_roll || [];
            
            // Texts
            result.text_count = r.texts ? r.texts.length : 0;
            result.texts = r.texts || [];
            result.text_roll = r.text_roll || [];
            
            return result;
        })
    };
    const blob = new Blob([JSON.stringify(data, null, 4)], {type: 'application/json'});
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'layout.json';
    a.click();
};

document.getElementById('importJsonBtn').onclick = () => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';
    input.onchange = (e) => {
        const file = e.target.files[0];
        const reader = new FileReader();
        reader.onload = (event) => {
            try {
                const data = JSON.parse(event.target.result);
                if (data.rectangles && Array.isArray(data.rectangles)) {
                    // 确保每个rectangle都有完整的字段
                    rectangles = data.rectangles.map(r => ({
                        index: r.index !== undefined ? r.index : 0,
                        is_mother: r.is_mother || "non",
                        Group: r.Group || [],
                        x_: r.x_ !== undefined ? r.x_ : 0,
                        y_: r.y_ !== undefined ? r.y_ : 0,
                        width_: r.width_ !== undefined ? r.width_ : 0.1,
                        height_: r.height_ !== undefined ? r.height_ : 0.1,
                        focus_mode: r.focus_mode !== undefined ? r.focus_mode : 0,
                        focus_icon: r.focus_icon || "nail",
                        on_confirm_action: r.on_confirm_action || "open_menu",
                        icon_count: r.icon_count !== undefined ? r.icon_count : (r.icons ? r.icons.length : 0),
                        icons: r.icons || [],
                        icon_roll: r.icon_roll || [],
                        text_count: r.text_count !== undefined ? r.text_count : (r.texts ? r.texts.length : 0),
                        texts: r.texts || [],
                        text_roll: r.text_roll || []
                    }));
                    selectedIndex = -1;
                    render();
                    updateRectList();
                    showProperties();
                    alert(`Successfully imported ${rectangles.length} rectangles`);
                } else {
                    alert('Invalid JSON format: Missing rectangles array');
                }
            } catch (err) {
                alert('Error parsing JSON: ' + err.message);
            }
        };
        reader.readAsText(file);
    };
    input.click();
};

// Mouse interactions
canvas.onmousedown = (e) => {
    const rect = canvas.getBoundingClientRect();
    const x = (e.clientX - rect.left);
    const y = (e.clientY - rect.top);
    
    isDragging = false;
    dragTarget = null;
    dragItemIndex = -1;
    
    // 检查是否点击了某个矩形内的元素
    for(let i = rectangles.length - 1; i >= 0; i--) {
        const r = rectangles[i];
        const rx = r.x_ * SCREEN_WIDTH;
        const ry = r.y_ * SCREEN_HEIGHT;
        const rw = r.width_ * SCREEN_WIDTH;
        const rh = r.height_ * SCREEN_HEIGHT;
        
        // 检查是否在矩形范围内
        if (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh) {
            // 检查是否点击了图标
            if (r.icons) {
                for(let ii = 0; ii < r.icons.length; ii++) {
                    const icon = r.icons[ii];
                    const ix = rx + (icon.rel_x * rw);
                    const iy = ry + (icon.rel_y * rh);
                    const hitRadius = 8; // 点击半径
                    
                    if (Math.abs(x - ix) <= hitRadius && Math.abs(y - iy) <= hitRadius) {
                        selectedIndex = i;
                        pushUndoSnapshot();
                        isDragging = true;
                        dragTarget = 'icon';
                        dragItemIndex = ii;
                        startX = x;
                        startY = y;
                        selectRect(i);
                        return;
                    }
                }
            }
            
            // 检查是否点击了图标滚动
            if (r.icon_roll) {
                for(let ii = 0; ii < r.icon_roll.length; ii++) {
                    const roll = r.icon_roll[ii];
                    const ix = rx + (roll.rel_x * rw);
                    const iy = ry + (roll.rel_y * rh);
                    const hitRadius = 8;
                    
                    if (Math.abs(x - ix) <= hitRadius && Math.abs(y - iy) <= hitRadius) {
                        selectedIndex = i;
                        pushUndoSnapshot();
                        isDragging = true;
                        dragTarget = 'icon_roll';
                        dragItemIndex = ii;
                        startX = x;
                        startY = y;
                        selectRect(i);
                        return;
                    }
                }
            }
            
            // 检查是否点击了静态文本
            if (r.texts) {
                for(let ii = 0; ii < r.texts.length; ii++) {
                    const text = r.texts[ii];
                    const tx = rx + (text.rel_x * rw);
                    const ty = ry + (text.rel_y * rh);
                    const hitRadius = 8;
                    
                    if (Math.abs(x - tx) <= hitRadius && Math.abs(y - ty) <= hitRadius) {
                        selectedIndex = i;
                        pushUndoSnapshot();
                        isDragging = true;
                        dragTarget = 'text';
                        dragItemIndex = ii;
                        startX = x;
                        startY = y;
                        selectRect(i);
                        return;
                    }
                }
            }
            
            // 检查是否点击了文本滚动
            if (r.text_roll) {
                for(let ii = 0; ii < r.text_roll.length; ii++) {
                    const text = r.text_roll[ii];
                    const tx = rx + (text.rel_x * rw);
                    const ty = ry + (text.rel_y * rh);
                    const hitRadius = 8;
                    
                    if (Math.abs(x - tx) <= hitRadius && Math.abs(y - ty) <= hitRadius) {
                        selectedIndex = i;
                        pushUndoSnapshot();
                        isDragging = true;
                        dragTarget = 'text_roll';
                        dragItemIndex = ii;
                        startX = x;
                        startY = y;
                        selectRect(i);
                        return;
                    }
                }
            }
            
            // 如果没有点击任何元素，只选中矩形
            selectRect(i);
            return;
        }
    }
    
    // 没有点击任何东西
    selectedIndex = -1;
    showProperties();
};

// 鼠标移动处理（检测可拖动元素并改变光标）
canvas.onmousemove = (e) => {
    const rect = canvas.getBoundingClientRect();
    const x = (e.clientX - rect.left);
    const y = (e.clientY - rect.top);
    
    // 如果正在拖动，处理拖动逻辑
    if (isDragging && selectedIndex !== -1 && dragTarget !== null) {
        const r = rectangles[selectedIndex];
        const rx = r.x_ * SCREEN_WIDTH;
        const ry = r.y_ * SCREEN_HEIGHT;
        const rw = r.width_ * SCREEN_WIDTH;
        const rh = r.height_ * SCREEN_HEIGHT;
        
        let targetX = x;
        let targetY = y;
        
        // 对齐吸附逻辑
        if (alignmentRef) {
            const snapThreshold = 5; // 吸附阈值（像素）
            
            // 检查垂直对齐（吸附到X轴）
            if (Math.abs(x - alignmentRef.x) < snapThreshold) {
                targetX = alignmentRef.x;
            }
            
            // 检查水平对齐（吸附到Y轴）
            if (Math.abs(y - alignmentRef.y) < snapThreshold) {
                targetY = alignmentRef.y;
            }
        }
        
        // 计算相对于矩形的位置（0-1范围）
        let rel_x = (targetX - rx) / rw;
        let rel_y = (targetY - ry) / rh;
        
        // 限制在矩形范围内
        rel_x = Math.max(0, Math.min(1, rel_x));
        rel_y = Math.max(0, Math.min(1, rel_y));
        
        // 根据拖动目标更新位置
        if (dragTarget === 'icon') {
            r.icons[dragItemIndex].rel_x = rel_x;
            r.icons[dragItemIndex].rel_y = rel_y;
        } else if (dragTarget === 'icon_roll') {
            r.icon_roll[dragItemIndex].rel_x = rel_x;
            r.icon_roll[dragItemIndex].rel_y = rel_y;
        } else if (dragTarget === 'text') {
            r.texts[dragItemIndex].rel_x = rel_x;
            r.texts[dragItemIndex].rel_y = rel_y;
        } else if (dragTarget === 'text_roll') {
            r.text_roll[dragItemIndex].rel_x = rel_x;
            r.text_roll[dragItemIndex].rel_y = rel_y;
        }
        
        // 如果拖动的是对齐参考点本身，更新参考点位置
        if (alignmentRef && alignmentRef.rectIndex === selectedIndex && 
            alignmentRef.itemIndex === dragItemIndex && alignmentRef.type === dragTarget) {
            alignmentRef.x = rx + (rel_x * rw);
            alignmentRef.y = ry + (rel_y * rh);
        }
        
        render();
        showProperties();
        return;
    }
    
    // 检测鼠标是否悬停在可拖动元素上
    let isOverDraggable = false;
    
    for(let i = rectangles.length - 1; i >= 0; i--) {
        const r = rectangles[i];
        const rx = r.x_ * SCREEN_WIDTH;
        const ry = r.y_ * SCREEN_HEIGHT;
        const rw = r.width_ * SCREEN_WIDTH;
        const rh = r.height_ * SCREEN_HEIGHT;
        
        if (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh) {
            const hitRadius = 8;
            
            // 检查图标
            if (r.icons) {
                for(let ii = 0; ii < r.icons.length; ii++) {
                    const icon = r.icons[ii];
                    const ix = rx + (icon.rel_x * rw);
                    const iy = ry + (icon.rel_y * rh);
                    if (Math.abs(x - ix) <= hitRadius && Math.abs(y - iy) <= hitRadius) {
                        isOverDraggable = true;
                        break;
                    }
                }
            }
            
            // 检查图标滚动
            if (!isOverDraggable && r.icon_roll) {
                for(let ii = 0; ii < r.icon_roll.length; ii++) {
                    const roll = r.icon_roll[ii];
                    const ix = rx + (roll.rel_x * rw);
                    const iy = ry + (roll.rel_y * rh);
                    if (Math.abs(x - ix) <= hitRadius && Math.abs(y - iy) <= hitRadius) {
                        isOverDraggable = true;
                        break;
                    }
                }
            }
            
            // 检查静态文本
            if (!isOverDraggable && r.texts) {
                for(let ii = 0; ii < r.texts.length; ii++) {
                    const text = r.texts[ii];
                    const tx = rx + (text.rel_x * rw);
                    const ty = ry + (text.rel_y * rh);
                    if (Math.abs(x - tx) <= hitRadius && Math.abs(y - ty) <= hitRadius) {
                        isOverDraggable = true;
                        break;
                    }
                }
            }
            
            // 检查文本滚动
            if (!isOverDraggable && r.text_roll) {
                for(let ii = 0; ii < r.text_roll.length; ii++) {
                    const text = r.text_roll[ii];
                    const tx = rx + (text.rel_x * rw);
                    const ty = ry + (text.rel_y * rh);
                    if (Math.abs(x - tx) <= hitRadius && Math.abs(y - ty) <= hitRadius) {
                        isOverDraggable = true;
                        break;
                    }
                }
            }
            
            if (isOverDraggable) break;
        }
    }
    
    // 更新光标样式
    canvas.style.cursor = isOverDraggable ? 'move' : 'default';
};

window.onmousemove = (e) => {
    // 此函数已被上面的 canvas.onmousemove 替代
};

window.onmouseup = () => {
    isDragging = false;
    dragTarget = null;
    dragItemIndex = -1;
    updateRectList();
};

// Keyboard listener for Ctrl+Z (single-step undo)
window.addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && (e.key === 'z' || e.key === 'Z')) {
        e.preventDefault();
        undoOnce();
    }
});

init();
